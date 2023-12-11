#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <ucontext.h>
#include <unistd.h>
#include "sut.h"
#include "queue.h"

#define STACK_SIZE 1024 * 1024

// Task Control Block
typedef struct TCB
{
    ucontext_t *context;
};

// Array of TCBs used to free memory after sut_shutdown()
struct TCB tcbs[30];

// Queues for C-EXEC and I-EXEC
struct queue ready_queue;
struct queue wait_queue;

// Time to sleep when checking queues
struct timespec ts = {0, 300000};

bool shutdown = false; // Indicates if kernel threads should break

// Kernel threads
pthread_t *c_thread;
pthread_t *i_thread;

// Mutexes for C-EXEC and I-EXEC threads
pthread_mutex_t c_lock;
pthread_mutex_t i_lock;

// Parent contexts managers for C-EXEC and I-EXEC threads
ucontext_t *c_manager;
ucontext_t *i_manager;

// Current queue head node
struct queue_entry *next_node_c;
struct queue_entry *next_node_i;

// Queue nodes to put in the queue
struct queue_entry *queue_node_c;
struct queue_entry *queue_node_i;

// TCBs for the currently running processes
struct TCB *c_tcb;
struct TCB *i_tcb;

/*--- C-EXEC Manager Function ---*/
void *c_exec_manager()
{
    while (1) // infinite loop till we call sut_shutdown()
    {
        if (c_manager != NULL) // save parent context in order to return after task
            getcontext(c_manager);

        pthread_mutex_lock(&c_lock); // lock the c_exec thread

        // Check if the ready queue is empty
        if (queue_peek_front(&ready_queue) == NULL)
        {
            nanosleep((const struct timespec[]){{0, 10000000000L}}, NULL); // sleep for a bit
        }

        else // when the queue is not empty
        {
            next_node_c = queue_pop_head(&ready_queue); // get the head of the ready queue

            c_tcb = (struct TCB *)next_node_c->data;
            c_tcb->context->uc_link = c_manager; // set the link to the parent context

            //  Unlock ready queue and set the context to the user thread
            pthread_mutex_unlock(&c_lock);
            setcontext(c_tcb->context);
        }

        // If the ready queue is empty, we check if we are shutting down and if so, we break the loop
        pthread_mutex_unlock(&c_lock);
        if (shutdown)
        {
            break;
        }
    }
}

void *i_exec_manager()
{
    while (1)
    {
        if (i_manager != NULL) // save parent context in order to return after task
            getcontext(i_manager);

        pthread_mutex_lock(&i_lock);
        // Check if the ready queue is empty
        if (queue_peek_front(&wait_queue) == NULL)
        {
            nanosleep((const struct timespec[]){{0, 10000000000L}}, NULL); // sleep for a bit
        }

        else // when the queue is not empty
        {
            next_node_i = queue_pop_head(&wait_queue); // get the head of the wait queue

            i_tcb = (struct TCB *)next_node_i->data;
            i_tcb->context->uc_link = i_manager; // set the user-thread link to the parent context

            // Run the next task
            setcontext(i_tcb->context);
        }

        // If the wait queue is empty, we check if we are shutting down and if so, we break the loop
        pthread_mutex_unlock(&i_lock);
        if (shutdown)
        {
            break;
        }
    }
}

/*--- Thread Library initialization function ---*/
void sut_init()
{
    shutdown = false; // Initialize shutdown flag

    // Initialize queues
    ready_queue = queue_create();
    wait_queue = queue_create();
    queue_init(&ready_queue);
    queue_init(&wait_queue);

    // Initialize tcbs
    for (int i = 0; i < 30; i++)
    {
        tcbs[i].context = NULL;
    }

    // Initialize our mutex locks
    pthread_mutex_init(&c_lock, NULL);
    pthread_mutex_init(&i_lock, NULL);

    // Initialize c_manager threads
    c_manager = (ucontext_t *)malloc(sizeof(ucontext_t));
    char *stack_c = (char *)malloc(STACK_SIZE);
    c_manager->uc_stack.ss_sp = stack_c;
    c_manager->uc_stack.ss_size = STACK_SIZE;
    c_manager->uc_stack.ss_flags = 0;
    c_manager->uc_link = NULL;

    // Initialize i_manager thread
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
{ // Create user thread context for the task
    ucontext_t *ucp = (ucontext_t *)malloc(sizeof(ucontext_t));
    if (getcontext(ucp) != 0)
        return false;
    char *stack = (char *)malloc(STACK_SIZE);
    ucp->uc_stack.ss_sp = stack;
    ucp->uc_stack.ss_size = STACK_SIZE;
    ucp->uc_stack.ss_flags = 0;
    ucp->uc_link = NULL;

    makecontext(ucp, fn, 0);                                    // initialize the context to run the desired function
    struct TCB *tcb = (struct TCB *)malloc(sizeof(struct TCB)); // create a new TCB
    tcb->context = ucp;                                         // set the context of the TCB to the user thread context

    for (int i = 0; i < 30; i++) // add the TCB to the tcbs array at the first available index
    {
        if (tcbs[i].context == NULL)
        {
            tcbs[i] = *tcb;
            break;
        }
    }

    pthread_mutex_lock(&c_lock);                   // lock the c_exec thread
    queue_node_c = queue_new_node(tcb);            // create a new queue node for the task
    queue_insert_tail(&ready_queue, queue_node_c); // insert the new TCB into the ready queue
    pthread_mutex_unlock(&c_lock);                 // unlock
    return true;
}

/*--- Function to change current process in C-Exec to next in queue, current context is saved and put back in queue ---*/
void sut_yield()
{
    pthread_mutex_lock(&c_lock);                   // lock the c_exec thread
    queue_node_c = queue_new_node(c_tcb);          // Create a new queue node
    queue_insert_tail(&ready_queue, queue_node_c); // insert the current TCB into the ready queue
    pthread_mutex_unlock(&c_lock);                 // unlock

    swapcontext(c_tcb->context, c_manager); // go back to the parent context
}

/*--- Function to exit the current thread and go back to parent, current context is not saved ---*/
void sut_exit()
{
    // printf("Exiting\n");
    setcontext(c_manager); // go back to the parent context
}

/*--- I-EXEC Functions ---*/

/*--- Helper function to insert process from C_EXEC to I_EXEC wait queue ---*/
void insert_to_wait_queue()
{
    pthread_mutex_lock(&i_lock);                  // lock the i_exec thread
    queue_node_i = queue_new_node(c_tcb);         // Create a new queue node with the current TCB of the c_exec thread
    queue_insert_tail(&wait_queue, queue_node_i); // insert the current TCB into the wait queue
    pthread_mutex_unlock(&i_lock);                // unlock the i_exec thread
    swapcontext(i_tcb->context, c_manager);       // go back to the parent context
}

/*--- Helper Function to handle returning from IO ---*/
void return_from_io()
{
    pthread_mutex_unlock(&i_lock); // lock the c_exec thread
    pthread_mutex_lock(&c_lock);   // lock the c_exec thread

    queue_node_i = queue_new_node(i_tcb); // Create a new queue node

    queue_insert_tail(&ready_queue, queue_node_i); // insert back to ready queue
    pthread_mutex_unlock(&c_lock);                 // unlock
    swapcontext(i_tcb->context, i_manager);        // go back to the parent context
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
    // Wait for all tasks to finish
    while (queue_peek_front(&ready_queue) != NULL && queue_peek_front(&wait_queue) != NULL)
    {
        nanosleep((const struct timespec[]){{0, 10000000000L}}, NULL); // sleep for a bit
    }

    shutdown = true;

    // Join kernel threads
    pthread_join(*c_thread, NULL);
    pthread_join(*i_thread, NULL);

    // Free memory
    free(c_thread);
    free(i_thread);
    free(c_manager);
    free(i_manager);

    // Free our array of TCBs
    for (int i = 0; i < 30; i++)
    {
        if (tcbs[i].context != NULL)
        {
            free(tcbs[i].context->uc_stack.ss_sp);
            free(tcbs[i].context);
        }
    }
}