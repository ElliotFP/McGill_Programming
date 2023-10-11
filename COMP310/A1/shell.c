// TODO: Remove jobs from the list when they are finished
// TODO: Add fallbacks to removeJob and getJobById if the job is not found

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

// A struct to represent a process that is running
typedef struct Job
{
    pid_t pid;         // Process ID
    int jobID;         // Job ID
    char command[256]; // Command that started this process
    struct Job *next;  // Pointer to the next job
} Job;

Job *job_list = NULL; // global pointer to the job list
int globalJobID = 0;  // global jobID counter

// Function to add a new job to the job list, it creates a new job and adds it to the front of the list
void addJob(pid_t pid, const char *command)
{
    Job *newJob = malloc(sizeof(Job)); // Allocate memory for the new job
    newJob->pid = pid;
    newJob->jobID = globalJobID++;                                  // increment globalJobID counter
    strncpy(newJob->command, command, sizeof(newJob->command) - 1); // Copy the command into the new job
    newJob->next = job_list;                                        // Set the next pointer to the current job list
    job_list = newJob;
}

// Remove a job from the job list using its PID, same as removing a node from a linked list
void removeJob(pid_t pid)
{
    Job *current = job_list;
    Job *previous = NULL;
    while (current)
    { // iterate through the list until we find the job with the given PID
        if (current->pid == pid)
        {
            if (previous)
            {
                previous->next = current->next;
            }
            else
            {
                job_list = current->next;
            }
            free(current);
            return;
        }
        previous = current; // move to the next job
        current = current->next;
    }
}

// Function to get the job based on its jobID
Job *getJobById(int jobID)
{
    Job *current = job_list;
    while (current) // iterate through the list until you get the correct jobID
    {
        if (current->jobID == jobID)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Function to print all jobs in the job list
void printJobs()
{
    Job *current = job_list;
    while (current) // iterate through the list and print all jobs
    {
        printf("PID: %d, Command: %s\n", current->pid, current->command);
        current = current->next;
    }
}

int getcmd(char *prompt, char *args[], int *background)
{
    int length, i = 0;
    char *token, *loc;
    char *line = NULL;
    size_t linecap = 0;
    printf("%s", prompt);
    length = getline(&line, &linecap, stdin);

    if (length <= 0)
    {
        free(line);
        exit(-1);
    }

    // Check if background is specified..
    if ((loc = index(line, '&')) != NULL)
    {
        *background = 1;
        *loc = ' ';
    }
    else
    {
        *background = 0;
    }

    // get prompt and arguments
    while ((token = strsep(&line, " \t\n")) != NULL)
    {
        for (int j = 0; j < strlen(token); j++)
            if (token[j] <= 32)
                token[j] = '\0';
        if (strlen(token) > 0)
            args[i++] = token;
    }

    args[i] = NULL; // Terminate the args array

    free(line);
    return i;
}

int main(void)
{
    char *args[20] = {NULL};
    int bg;

    while (1)
    {
        bg = 0;
        int cnt = getcmd("\n>> ", args, &bg);

        if (args[0] == NULL)
        {
            printf("No command entered.\n");
            continue;
        }

        if (strcmp(args[0], "echo") == 0) // echo command
        {
            if (args[1] == NULL)
            {
                printf("No arguments given");
            }
            else
            {
                // I thought we had to check for files, but turns out we don't, this code works though
                // struct stat st;
                // // Check if the argument is a file
                // if (stat(args[1], &st) == 0 && S_ISREG(st.st_mode))
                // {
                //     FILE *file = fopen(args[1], "r"); // open the file and read the contents
                //     if (file == NULL)
                //     {
                //         perror("Error opening file");
                //     }
                //     else
                //     {
                //         char ch;
                //         while ((ch = fgetc(file)) != EOF) // print the contents of the file
                //         {
                //             putchar(ch);
                //         }
                //         fclose(file);
                //     }
                // }
                // else
                // {
                // It's not a file, so just print the argument
                printf("%s\n", args[1]);
                // }
            }
        }
        else if (strcmp(args[0], "cd") == 0) // cd command
        {
            if (args[1] == NULL) // if no argument is given then print the current directory
            {
                char cwd[1024]; // buffer to store the current working directory
                if (getcwd(cwd, sizeof(cwd)) != NULL)
                {
                    printf("%s\n", cwd);
                }
                else
                {
                    perror("pwd failed");
                }
            }
            else
            {
                chdir(args[1]);
            }
        }
        else if (strcmp(args[0], "pwd") == 0) // pwd command
        {
            char cwd[1024]; // buffer to store the current working directory
            if (getcwd(cwd, sizeof(cwd)) != NULL)
            {
                printf("%s\n", cwd);
            }
            else
            {
                perror("pwd failed");
            }
        }
        else if (strcmp(args[0], "exit") == 0) // exit command
        {
            exit(0);
        }
        else if (strcmp(args[0], "fg") == 0) // fg command
        {
            if (args[1] == NULL)
            {
                printf("Please provide a job number.\n");
            }
            else
            {
                int jobID = atoi(args[1]);
                Job *jobToForeground = getJobById(jobID);
                if (jobToForeground)
                {
                    printf("%d (%s) with PID %d to foreground...\n", jobToForeground->jobID, jobToForeground->command, jobToForeground->pid);
                    waitpid(jobToForeground->pid, NULL, 0); // wait for the specific process
                    removeJob(jobToForeground->pid);        // Remove the job from list once it's done
                }
                else
                {
                    printf("Job number %d not found.\n", jobID);
                }
            }
        }
        else if (strcmp(args[0], "jobs") == 0) // jobs command
        {
            printJobs();
        }
        else
        {
            pid_t pid = fork();

            if (pid < 0)
            {
                // Fork failed
                perror("Fork failed");
                return 1;
            }

            // Child process
            if (pid == 0)
            {
                execvp(args[0], args);
            }
            else
            {
                if (bg)
                {
                    addJob(pid, args[0]);
                }
                if (!bg)
                {
                    // Wait for the child process to finish
                    wait(NULL);
                    printf("Child process has finished.\n");
                }
            }
        }
    }
    return 0;
}
