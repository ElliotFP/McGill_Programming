// Elliot Forcier-Poirier
// 260989602
// COMP 310 - Assignment 2 - Fall 2023
// McGill University - School of Computer Science

#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <ucontext.h>
#include "sut.h"
#include "queue.h"

#define STACK_SIZE 1024 * 1024 // Stack size for user threads

// Task Control Block
typedef struct TCB
{
    ucontext_t *context;
};

bool shutdown = false; // flag to know if kernel threads should break
int tasks = 0;         // Number of tasks left to complete

// currently running TCBs for C-EXEC and I-EXEC
struct TCB *c_tcb;
struct TCB *i_tcb;

// Queues for C-EXEC and I-EXEC
struct queue ready_queue;
struct queue wait_queue;

// Kernel threads
pthread_t *c_thread;
pthread_t *i_thread;

// Mutex locks for C-EXEC and I-EXEC threads
pthread_mutex_t c_lock;
pthread_mutex_t i_lock;
pthread_mutex_t task_num_lock; // Mutex lock for task number

// Parent contexts for C-EXEC and I-EXEC threads
ucontext_t *c_manager;
ucontext_t *i_manager;

/*--- C-EXEC Manager Function ---*/
void *c_exec_manager()
{
    while (1) // Infinite loop to keep the kernel thread running until shutdown
    {
        // Save c_manager context to return to when user thread is done
        if (c_manager != NULL)
            getcontext(c_manager);

        nanosleep((const struct timespec[]){{0, 500000}}, NULL); // sleep for a bit
        pthread_mutex_lock(&c_lock);                             // Critical Section

        if (queue_peek_front(&ready_queue) != NULL) // If ready queue is not empty
        {
            // Pop the first element and run the user thread
            struct queue_entry *next_task = queue_pop_head(&ready_queue);
            struct TCB *tcb = (struct TCB *)next_task->data;

            tcb->context->uc_link = c_manager; // Set the user-thread link to the parent context
            c_tcb = tcb;                       // Set the current TCB to the current C-EXEC TCB
            free(next_task);                   // Free the queue entry

            // Unlock ready queue and go to the user thread
            pthread_mutex_unlock(&c_lock);
            setcontext(tcb->context);
        }

        // If the queue is empty, we check if we should shutdown
        pthread_mutex_unlock(&c_lock);
        pthread_mutex_lock(&task_num_lock);
        if (shutdown)
        {
            pthread_mutex_unlock(&task_num_lock);
            break;
        }
        pthread_mutex_unlock(&task_num_lock);
    }
}

/*--- I-EXEC Manager Function ---*/
void *i_exec_manager()
{
    while (1) // Infinite loop to keep the kernel thread running until shutdown
    {
        // Save i_manager context to return to when user thread is done
        if (i_manager != NULL)
            getcontext(i_manager);

        nanosleep((const struct timespec[]){{0, 500000}}, NULL); // sleep for a bit
        pthread_mutex_lock(&i_lock);                             // Critical Section

        if (queue_peek_front(&wait_queue) != NULL) // If wait queue is not empty
        {
            // Pop the first element and run the user thread
            struct queue_entry *entry = queue_pop_head(&wait_queue);
            struct TCB *tcb = (struct TCB *)entry->data;

            tcb->context->uc_link = i_manager; // Set the user-thread link to the parent context
            i_tcb = tcb;                       // Set the current TCB to the current I-EXEC TCB
            free(entry);                       // Free the queue entry

            // Go to the user thread
            setcontext(tcb->context);
        }

        // If the queue is empty, we check if we should shutdown
        pthread_mutex_unlock(&i_lock);
        pthread_mutex_lock(&task_num_lock);
        if (shutdown)
        {
            pthread_mutex_unlock(&task_num_lock);
            break;
        }
        pthread_mutex_unlock(&task_num_lock);
    }
}

/*--- Thread Library initialization function ---*/
void sut_init()
{
    // Initialize queues
    ready_queue = queue_create();
    wait_queue = queue_create();
    queue_init(&ready_queue);
    queue_init(&wait_queue);

    // Initialize mutex locks
    pthread_mutex_init(&c_lock, NULL);
    pthread_mutex_init(&i_lock, NULL);
    pthread_mutex_init(&task_num_lock, NULL);

    // Initialize c_manager parent context
    c_manager = (ucontext_t *)malloc(sizeof(ucontext_t));
    char *stack_c = (char *)malloc(STACK_SIZE);
    c_manager->uc_stack.ss_sp = stack_c;
    c_manager->uc_stack.ss_size = STACK_SIZE;
    c_manager->uc_stack.ss_flags = 0;
    c_manager->uc_link = NULL;

    // Initialize i_manager parent context
    i_manager = (ucontext_t *)malloc(sizeof(ucontext_t));
    char *stack_i = (char *)malloc(STACK_SIZE);
    i_manager->uc_stack.ss_sp = stack_i;
    i_manager->uc_stack.ss_size = STACK_SIZE;
    i_manager->uc_stack.ss_flags = 0;
    i_manager->uc_link = NULL;

    // Initialize kernel threads
    c_thread = (pthread_t *)malloc(sizeof(pthread_t));
    i_thread = (pthread_t *)malloc(sizeof(pthread_t));

    // Create C-EXEC and I-EXEC threads
    if (pthread_create(c_thread, NULL, c_exec_manager, NULL) != 0)
    {
        perror("Failed to create C-EXEC thread");
    }
    if (pthread_create(i_thread, NULL, i_exec_manager, NULL) != 0)
    {
        perror("Failed to create I-EXEC thread");
    }
}

/*--- C-EXEC Functions ---*/

/*--- Function to create a new task ---*/
bool sut_create(sut_task_f fn)
{

    // Increment number of tasks
    pthread_mutex_lock(&task_num_lock);
    tasks++;
    pthread_mutex_unlock(&task_num_lock);

    // create a new context object for the task
    ucontext_t *ucp = (ucontext_t *)malloc(sizeof(ucontext_t));
    if (getcontext(ucp) != 0)
        return false;
    char *stack = (char *)malloc(sizeof(char) * STACK_SIZE);
    ucp->uc_stack.ss_sp = stack;
    ucp->uc_stack.ss_size = STACK_SIZE;
    ucp->uc_stack.ss_flags = 0;
    ucp->uc_link = NULL;

    makecontext(ucp, fn, 0); // initialize the context to run the desired function

    // create a new TCB for the task
    struct TCB *tcb = (struct TCB *)malloc(sizeof(struct TCB));
    tcb->context = ucp;

    // create a new queue node for the task
    struct queue_entry *new_node = queue_new_node(tcb);

    pthread_mutex_lock(&c_lock);               // lock the c_exec thread
    queue_insert_tail(&ready_queue, new_node); // insert the new TCB into the ready queue
    pthread_mutex_unlock(&c_lock);             // unlock

    return true;
}

/*--- Function to change current process in C-Exec to next in queue, current context is saved and put back in queue ---*/
void sut_yield()
{
    pthread_mutex_lock(&c_lock);                       // lock the c_exec thread
    struct queue_entry *entry = queue_new_node(c_tcb); // Create a new queue node
    queue_insert_tail(&ready_queue, entry);            // insert the current TCB into the ready queue
    pthread_mutex_unlock(&c_lock);                     // unlock

    swapcontext(c_tcb->context, c_manager); // go back to the parent context
}

/*--- Function to exit the current thread and go back to parent, current context is not saved ---*/
void sut_exit()
{
    pthread_mutex_lock(&task_num_lock);   // lock the task_num_lock
    tasks--;                              // decrement the number of tasks left
    pthread_mutex_unlock(&task_num_lock); // unlock the task_num_lock

    setcontext(c_manager); // switch back to the parent context
}

/*--- IO Functions ---*/

/*--- Helper function to insert process from C_EXEC to I_EXEC wait queue ---*/
void insert_to_wait_queue()
{
    pthread_mutex_lock(&i_lock); // lock the i_exec thread

    i_tcb = c_tcb;                                        // set the current TCB to the current C-EXEC TCB
    struct queue_entry *new_node = queue_new_node(i_tcb); // create a new queue node
    queue_insert_tail(&wait_queue, new_node);             // insert the current TCB into the wait queue
    pthread_mutex_unlock(&i_lock);                        // unlock the i_exec thread
    swapcontext(i_tcb->context, c_manager);               // go back to the parent context
}

/*--- Helper Function to handle returning from IO ---*/
void return_from_io()
{
    // queue_node_c = queue_new_node(current_tcb_i); // Create a new queue node
    struct queue_entry *new_node_ = queue_new_node(i_tcb);

    pthread_mutex_unlock(&i_lock);              // lock the c_exec thread
    pthread_mutex_lock(&c_lock);                // lock the c_exec thread
    queue_insert_tail(&ready_queue, new_node_); // insert back to ready queue
    pthread_mutex_unlock(&c_lock);              // unlock
    swapcontext(i_tcb->context, i_manager);     // go back to the parent context
}

/*--- Function to start IO operations with a given file ---*/
int sut_open(char *file_name)
{
    printf("Opening file\n");
    insert_to_wait_queue();                           // insert the current TCB into the wait queue
    int rv = open(file_name, O_RDWR | O_CREAT, 0666); // open the file
    return_from_io();                                 // return from IO
    return rv;
}

/*--- Function to close file after use ---*/
void sut_close(int fd)
{
    printf("Closing file\n");
    insert_to_wait_queue(); // insert the current TCB into the wait queue
    if (close(fd) != 0)     // close the file
        perror("Error: sut_close() failed");
    return_from_io(); // return from IO
}

/*--- Function to write into a provided file ---*/
void sut_write(int fd, char *buf, int size)
{
    printf("Writing to file\n");
    insert_to_wait_queue(); // insert the current TCB into the wait queue
    write(fd, buf, size);   // write to the file
    return_from_io();       // return from IO
}

/*--- Function to get data from a given file ---*/
char *sut_read(int fd, char *buf, int size)
{
    printf("Reading from file\n");
    insert_to_wait_queue(); // insert the current TCB into the wait queue
    read(fd, buf, size);    // read from the file into the provided buffer
    return_from_io();       // return from IO
    return buf;
}

/*--- Function to completely shut down the executors and terminates the program. ---*/
void sut_shutdown()
{
    // Wait for all tasks to complete
    while (1)
    {
        pthread_mutex_lock(&task_num_lock);
        if (tasks == 0)
            break;
        pthread_mutex_unlock(&task_num_lock);
    }

    shutdown = true;                      // Indicate that kernel threads should shut down
    pthread_mutex_unlock(&task_num_lock); // Unlock task_num_lock

    // Join kernel threads
    pthread_join(*c_thread, NULL);
    pthread_join(*i_thread, NULL);

    // Free memory
    free(c_thread);
    free(i_thread);
    free(c_manager);
    free(i_manager);
}
