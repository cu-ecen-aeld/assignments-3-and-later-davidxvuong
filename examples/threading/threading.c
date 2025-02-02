#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

// Per ChatGPT, this is now the POSIX-compliant way to sleep for a certain millisecond
static void sleep_ms(int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data *t = (struct thread_data *)thread_param;
    t->thread_complete_success = false;

    sleep_ms(t->sleep_ms);
    pthread_mutex_lock(t->mutex);
    sleep_ms(t->lock_hold_ms);
    pthread_mutex_unlock(t->mutex);
    t->thread_complete_success = true;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    struct thread_data *t = (struct thread_data *)malloc(sizeof(struct thread_data));

    t->mutex = mutex;
    t->sleep_ms = wait_to_obtain_ms;
    t->lock_hold_ms = wait_to_release_ms;

    if (pthread_create(thread, NULL, threadfunc, t) != 0)
    {
        return false;
    }

    return true;
}

