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

#define STACK_SIZE 1024 * 1024 // Stack size for user threads

// Structure used to store user thread contexts
typedef struct TCB
{
    ucontext_t *context;
};

// Array of TCBs used to free memory after sut_shutdown()
struct TCB tcbs[30];

// TCBs for currently running Compute (c) and I/O (i) threads
struct TCB *current_c_tcb;
struct TCB *current_i_tcb;

struct queue ready_queue; // C-EXEC queue
struct queue wait_queue;  // I-EXEC queue

// Time to sleep when checking queues
struct timespec ts = {0, 300000};

bool shutdown = false; // Indicates if kernel threads should break
int tasks_left = 0;    // Number of tasks left to complete

// Kernel threads ~ C-EXEC (c) and I-EXEC (i)
pthread_t *c_thread;
pthread_t *i_thread;

// Mutexes for C-EXEC and I-EXEC queues & (task number / shutdown) variables
pthread_mutex_t c_lock;
pthread_mutex_t i_lock;
pthread_mutex_t task_num_lock;

// Parent contexts for C-EXEC and I-EXEC threads
ucontext_t *parent_c;
ucontext_t *parent_i;

void *handle_c_thread()
{
    /*
     *   Ready queue handling and context switching
     */
    while (1)
    {
        // Save parent_c context to return to when user thread is done
        if (parent_c != NULL)
            getcontext(parent_c);
        nanosleep(&ts, NULL);
        // Wait for access to ready queue
        pthread_mutex_lock(&c_lock);
        // If ready queue is not empty, pop the first element and run the user thread
        if (queue_peek_front(&ready_queue) != NULL)
        {
            struct queue_entry *entry = queue_pop_head(&ready_queue);
            struct TCB *tcb = (struct TCB *)entry->data;
            // Set the user-thread link to the parent context
            tcb->context->uc_link = parent_c;
            current_c_tcb = tcb;
            free(entry);
            // Unlock ready queue and run the user thread
            pthread_mutex_unlock(&c_lock);
            setcontext(tcb->context);
        }
        // --- The following code runs if the ready queue is empty
        // Unlock ready queue and check if the kernel thread should shut down
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

void *handle_i_thread()
{
    /*
     *   Wait queue handling and context switching
     */
    while (1)
    {
        // Save parent_i context to return to when user thread is done
        if (parent_i != NULL)
            getcontext(parent_i);
        nanosleep(&ts, NULL);
        // Wait for access to wait queue
        pthread_mutex_lock(&i_lock);
        // If wait queue is not empty, pop the first element and run the user thread
        if (queue_peek_front(&wait_queue) != NULL)
        {
            struct queue_entry *entry = queue_pop_head(&wait_queue);
            struct TCB *tcb = (struct TCB *)entry->data;
            // Set the user-thread link to the parent context
            tcb->context->uc_link = parent_i;
            current_i_tcb = tcb;
            free(entry);
            // Run the user thread
            setcontext(tcb->context);
        }
        // --- The following code runs if the wait queue is empty
        // Unlock wait queue and check if the kernel thread should shut down
        pthread_mutex_unlock(&i_lock);
        pthread_mutex_lock(&task_num_lock);
        if (shutdown)
        {
            pthread_mutex_unlock(&task_num_lock);
            break;
        }
        pthread_mutex_unlock(&task_num_lock);
        nanosleep(&ts, &ts);
    }
}

void sut_init()
{
    /*
     *   Initialize data structures and threads
     */
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

    // Initialize mutexes
    pthread_mutex_init(&c_lock, NULL);
    pthread_mutex_init(&i_lock, NULL);
    pthread_mutex_init(&task_num_lock, NULL);

    // Initialize parent contexts
    parent_c = (ucontext_t *)malloc(sizeof(ucontext_t));
    char *stack_c = (char *)malloc(STACK_SIZE);
    parent_c->uc_stack.ss_sp = stack_c;
    parent_c->uc_stack.ss_size = STACK_SIZE;
    parent_c->uc_stack.ss_flags = 0;
    parent_c->uc_link = NULL;

    parent_i = (ucontext_t *)malloc(sizeof(ucontext_t));
    char *stack_i = (char *)malloc(STACK_SIZE);
    parent_i->uc_stack.ss_sp = stack_i;
    parent_i->uc_stack.ss_size = STACK_SIZE;
    parent_i->uc_stack.ss_flags = 0;
    parent_i->uc_link = NULL;

    // Initialize kernel threads
    c_thread = (pthread_t *)malloc(sizeof(pthread_t));
    i_thread = (pthread_t *)malloc(sizeof(pthread_t));
    pthread_create(c_thread, NULL, handle_c_thread, NULL);
    pthread_create(i_thread, NULL, handle_i_thread, NULL);
}

bool sut_create(sut_task_f fn)
{
    /*
     *   Create a new user thread
     */
    // Increment number of tasks left
    pthread_mutex_lock(&task_num_lock);
    tasks_left++;
    pthread_mutex_unlock(&task_num_lock);

    // Create user thread context
    ucontext_t *ucp = (ucontext_t *)malloc(sizeof(ucontext_t));
    if (getcontext(ucp) != 0)
        return false;
    char *stack = (char *)malloc(STACK_SIZE);
    ucp->uc_stack.ss_sp = stack;
    ucp->uc_stack.ss_size = STACK_SIZE;
    ucp->uc_stack.ss_flags = 0;
    ucp->uc_link = NULL;

    // Set user thread function
    makecontext(ucp, fn, 0);

    // Create TCB
    struct TCB *tcb = (struct TCB *)malloc(sizeof(struct TCB));
    tcb->context = ucp;
    for (int i = 0; i < 30; i++)
    {
        if (tcbs[i].context == NULL)
        {
            tcbs[i] = *tcb;
            break;
        }
    }

    // Add TCB to ready queue
    struct queue_entry *entry = queue_new_node(tcb);
    pthread_mutex_lock(&c_lock);
    queue_insert_tail(&ready_queue, entry);
    pthread_mutex_unlock(&c_lock);
    return true;
}

void sut_yield()
{
    /*
     *   Yield current user thread
     */
    // Add current user thread to ready queue
    struct queue_entry *entry = queue_new_node(current_c_tcb);
    pthread_mutex_lock(&c_lock);
    queue_insert_tail(&ready_queue, entry);
    pthread_mutex_unlock(&c_lock);

    // Save context of current user thread and switch back to parent thread
    swapcontext(current_c_tcb->context, parent_c);
}

void sut_exit()
{
    /*
     *   End current user thread
     */
    // Decrement number of tasks left
    pthread_mutex_lock(&task_num_lock);
    tasks_left--;
    pthread_mutex_unlock(&task_num_lock);

    // Switch back to parent thread
    setcontext(parent_c);
}
void sut_shutdown()
{
    /*
     *   Shutdown kernel threads
     */
    // Wait for all tasks to complete
    while (1)
    {
        pthread_mutex_lock(&task_num_lock);
        if (tasks_left == 0)
            break;
        pthread_mutex_unlock(&task_num_lock);
    }
    // Shutdown kernel threads
    shutdown = true;
    pthread_mutex_unlock(&task_num_lock);

    // Join kernel threads
    pthread_join(*c_thread, NULL);
    pthread_join(*i_thread, NULL);

    // Free memory
    free(c_thread);
    free(i_thread);
    free(parent_c);
    free(parent_i);
    for (int i = 0; i < 30; i++)
    {
        if (tcbs[i].context != NULL)
        {
            free(tcbs[i].context->uc_stack.ss_sp);
            free(tcbs[i].context);
        }
    }
}

void preIO()
{
    /*
     *   Set up user thread for I/O
     */
    // Add current user thread to wait queue
    pthread_mutex_lock(&i_lock);
    current_i_tcb = current_c_tcb;
    struct queue_entry *entry = queue_new_node(current_i_tcb);
    queue_insert_tail(&wait_queue, entry);
    pthread_mutex_unlock(&i_lock);
    swapcontext(current_i_tcb->context, parent_c);
}

void postIO()
{
    /*
     *   Set up user thread after compute
     */
    // Add current user thread to ready queue
    struct queue_entry *entry2 = queue_new_node(current_i_tcb);
    pthread_mutex_unlock(&i_lock);
    pthread_mutex_lock(&c_lock);
    queue_insert_tail(&ready_queue, entry2);
    pthread_mutex_unlock(&c_lock);
    swapcontext(current_i_tcb->context, parent_i);
}

int sut_open(char *fname)
{
    /*
     *   Open file
     *   Returns file descriptor or -1 if open fails
     */
    preIO();
    // --------- Back from wait ---------
    int val = open(fname, O_RDWR | O_CREAT, 0666);
    postIO();
    return val;
}

void sut_write(int fd, char *buf, int size)
{
    /*
     *   Write to file
     */
    preIO();
    // --------- Back from wait ---------
    write(fd, buf, size);
    postIO();
    return;
}
char *sut_read(int fd, char *buf, int size)
{
    /*
     *   Read from file
     *   Returns NULL if read fails
     */
    preIO();
    // --------- Back from wait ---------
    // buf = (char *)malloc(size);
    read(fd, buf, size);
    postIO();
    return buf;
}

void sut_close(int fd)
{
    /*
     *   Close file
     */
    preIO();
    // --------- Back from wait ---------
    if (close(fd) < 0)
    {
        perror("Error: sut_close() failed");
    }
    postIO();
    return;
}

// int main(int argc, char *argv[])
// {
//     sut_init();
//     sut_create(hello1);
//     sut_create(hello2);
//     sut_shutdown();
// }x