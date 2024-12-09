#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)
#define NO_ERROR 0

void* threadfunc(void* thread_param)
{

    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    usleep(thread_func_args->wait_to_obtain_ms*1000);
    int rc;

    // wait, obtain mutex, wait, release mutex as described by thread_data structure
    //get the mutex 
    rc=pthread_mutex_lock(thread_func_args->mutex);
    // check mutex lock rc 
    if (NO_ERROR!=rc)
    {
        ERROR_LOG("\nFailed to obtain mutex, error code: %d \n", rc);
        thread_func_args->thread_complete_success = false;
        return NULL;
    }
    else
    {
        DEBUG_LOG("\nMutex obtained succssfully\n");
    }

    // sleep for another specified time
    rc=usleep(thread_func_args->wait_to_release_ms * 1000);

    // release the mutexf
    pthread_mutex_unlock(thread_func_args->mutex);
    // check mutex unlock rc 
    if (NO_ERROR!=rc)
    {
        ERROR_LOG("\nFailed to release mutex, error code: %d \n", rc);
        thread_func_args->thread_complete_success = false;
        return NULL;
    }
    else
    {
        DEBUG_LOG("\nMutex released succssfully\n");
    }

    // Set thread_complete_success to true
    thread_func_args->thread_complete_success = true;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    struct thread_data* thread_func_args = (struct thread_data*)malloc(sizeof(struct thread_data));
    int rc;
     // Initialize the data
    thread_func_args->mutex = mutex;
    thread_func_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_func_args->wait_to_release_ms = wait_to_release_ms;

    rc = pthread_create(thread, NULL, threadfunc, (void*)thread_func_args);

    if (NO_ERROR != rc)
    {

        ERROR_LOG("\nFailed to create thread, error code: %d \n", rc);
        return false;
    }
    else
    {
        DEBUG_LOG("\nThread created succssfully\n");
    }

    return true;
}

