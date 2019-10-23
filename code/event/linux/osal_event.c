/**

  @file    event/linux/osal_event.c
  @brief   Creating, deleting, setting and waiting for events.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This file implements event related functionality for Windows. Generally events are used for
  a thread wait until it needs to do something.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"

#if OSAL_MULTITHREAD_SUPPORT

#define _GNU_SOURCE
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
// #include <semaphore.h>


#include <stdio.h> // FOR TESTING

/* THIS SHOULE NOT BE NEEDED, WHY NEEDED ? */
int pipe2(int pipefd[2], int flags);

typedef struct osalPosixEvent
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    os_boolean signaled;

    /** Pipe file descriptors to use event to interrupt socket select().
     */
    int pipefd[2];
}
osalPosixEvent;



/**
****************************************************************************************************

  @brief Create an event.
  @anchor osal_event_create

  The osal_event_create() function creates an event. An event is used to suspend a thread to wait
  for signal from an another thread of the same process. The new event is initially in nonsignaled
  state, so immediate osal_event_wait() for new event will block execution.

  Resource monitor's event count is incremented, if resource monitor is enabled.

  @return  evnt Event pointer. If the function fails, it returns OS_NULL.

****************************************************************************************************
*/
osalEvent osal_event_create(
    void)
{
    osalPosixEvent *pe;
    pthread_condattr_t attrib;

    /* Allocate event handle stricture and mark it initially not signaled. Pipes are not
     * created by default.
     */
    pe = malloc(sizeof(osalPosixEvent));
    pe->signaled = OS_FALSE;
    pe->pipefd[0] = pe->pipefd[1] =  -1;

    /* Create mutex to access the event
     */
    if (pthread_mutex_init(&pe->mutex, NULL))
    {
        osal_debug_error("osal_event.c: pthread_mutex_init() failed");
        return OS_NULL;
    }

    /* Setup condition attributes
     */
    pthread_condattr_init(&attrib);
    pthread_condattr_setclock(&attrib, CLOCK_MONOTONIC);

    /* Create condition variable.
     */
    if (pthread_cond_init(&pe->cond, &attrib))
    {
        osal_debug_error("osal_event.c: pthread_cond_init() failed");
        return OS_NULL;
    }

    pthread_condattr_destroy(&attrib);


    /* Inform resource monitor that event has been created and return the event pointer.
     */
    osal_resource_monitor_increment(OSAL_RMON_EVENT_COUNT);
    return (osalEvent)pe;
}


/**
****************************************************************************************************

  @brief Delete an event.
  @anchor osal_event_delete

  The osal_event_delete() function deletes an event which was created by osal_event_create()
  function. Resource monitor's event count is decremented, if resource monitor is enabled.

  @param   evnt Pointer to event to delete. 

  @return  None.

****************************************************************************************************
*/
void osal_event_delete(
    osalEvent evnt)
{
    osalPosixEvent *pe;

    if (evnt == OS_NULL)
    {
        osal_debug_error("osal_event_delete: NULL argument");
        return;
    }

    pe = (osalPosixEvent*)evnt;

    /* If we have pipe, close it.
     */
    if (pe->pipefd[0] != -1)
    {
        close(pe->pipefd[0]);
        close(pe->pipefd[1]);
    }

    /* Delete condition variable.
     */
    if (pthread_cond_destroy(&pe->cond))
    {
        osal_debug_error("osal_event.c: pthread_cond_destroy() failed");
        return;
    }

    /* Delete mutex.
     */
    if (pthread_mutex_destroy(&pe->mutex))
    {
        osal_debug_error("osal_event.c: pthread_mutex_destroy() failed");
        return;
    }

    /* Free memory allocated for the event structure and inform resource monitor
       that event has been deleted.
     */
    free(pe);
    osal_resource_monitor_decrement(OSAL_RMON_EVENT_COUNT);

}


/**
****************************************************************************************************

  @brief Set an event.
  @anchor osal_event_set

  The osal_event_set() function signals an event, which will allow waiting thread to proceed.
  The event object remains signaled until a single waiting thread is released, at which time
  the system sets the state to nonsignaled. If no threads are waiting, the event object's state
  remains signaled, and first following osal_event_wait() call will return immediately.

  @param   evnt Event pointer returned by osal_event_create() function.
  @return  None.

****************************************************************************************************
*/
void osal_event_set(
    osalEvent evnt)
{
    osalPosixEvent *pe;

    if (evnt == OS_NULL)
    {
        osal_debug_error("osal_event_set: NULL argument");
        return;
    }

    pe = (osalPosixEvent*)evnt;

    /* Lock event structure.
     */
    pthread_mutex_lock(&pe->mutex);

    /* If event is already signaled, there is not much to do.
     */
    if (pe->signaled)
    {
        pthread_mutex_unlock(&pe->mutex);
        return;
    }

    /* Mark event as signaled.
     */
    pe->signaled = OS_TRUE;

    /* If we are interrupting socket select().
     */
    if (pe->pipefd[0] != -1)
    {
        if (write(pe->pipefd[1], "\n", 1) != 1)
        {
            osal_debug_error("pipe write failed");
        }
    }

    /* Set condition variable.
     */
    if (pthread_cond_signal(&pe->cond))
    {
        osal_debug_error("osal_event.c: pthread_cond_signal failed");
    }

    /* Unlock event.
     */
    pthread_mutex_unlock(&pe->mutex);
}


/**
****************************************************************************************************

  @brief Wait for an event.
  @anchor osal_event_wait

  The osal_event_wait() function causes this thread to wait for event. If the event's state is
  nonsignaled, the calling thread enters an efficient wait state. The is done by operating
  system and expected to consume very little processor time while waiting. If event state
  is signaled either before the function call or during wait interval function returns
  OSAL_SUCCESS. When the function returns the event is always cleared to non signaled state.

  NOTE: Currently only timeout values 0 and OSAL_EVENT_INFINITE (-1) are supported.
  I hope others would not be needed.

  @param   evnt Event pointer returned by osal_event_create() function.
  @param   timeout_ms Wait timeout. If event is not signaled within this time, then the
           function will return OSAL_STATUS_EVENT_TIMEOUT. To wait infinetly give
           OSAL_EVENT_INFINITE (-1) here. To check event state or to reset event to non
           signaled state without waiting set timeout_ms to 0.

  @return  If the event was signaled, either before the osal_event_wait call or during
           wait interval, the function will return OSAL_SUCCESS (0). If the function timed
           out and the event remained unsignaled, it will return OSAL_STATUS_EVENT_TIMEOUT.
           Other values indicate failure, typically OSAL_STATUS_EVENT_FAILED.
           See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_event_wait(
    osalEvent evnt,
    os_int timeout_ms)
{
    osalPosixEvent *pe;
    osalStatus s;

    if (evnt == OS_NULL)
    {
        osal_debug_error("osal_event_wait: NULL argument");
        return OSAL_STATUS_EVENT_FAILED;
    }

    /* Cast posix event pointer and lock event.
     */
    pe = (osalPosixEvent*)evnt;
    pthread_mutex_lock(&pe->mutex);

    /* If event is already signaled or immediate return regardless is requested,
       set event to unsignald state and return OSAL SUCCESS or OSAL_STATUS_EVENT_TIMEOUT.
     */
    if (pe->signaled || timeout_ms == 0)
    {
        s = (pe->signaled ? OSAL_SUCCESS : OSAL_STATUS_EVENT_TIMEOUT);
        pe->signaled = OS_FALSE;
        pthread_mutex_unlock(&pe->mutex);
        return s;
    }

    /* Wait for event.
     */
    do
    {
        pthread_cond_wait(&pe->cond, &pe->mutex);
    }
    while (!pe->signaled);

    pe->signaled = OS_FALSE;
    pthread_mutex_unlock(&pe->mutex);
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Get pipe fd.
  @anchor osal_event_pipefd

  The osal_event_pipefd() function..

  @param   evnt Event pointer returned by osal_event_create() function.
  @return  Pipe fd (handle) which can be used to interrupt socket() select when event
           is triggered. -1 if the function failed.

****************************************************************************************************
*/
int osal_event_pipefd(
    osalEvent evnt)
{
    osalPosixEvent *pe;

    if (evnt == OS_NULL)
    {
        osal_debug_error("osal_event_pipefd: NULL argument");
        return -1;
    }

    /* Cast posix event pointer.
     */
    pe = (osalPosixEvent*)evnt;

    if (pe->pipefd[0] == -1)
    {
        printf ("Initializing pipe...");

        // if (pipe(pe->pipefd) == -1)
        if (pipe2(pe->pipefd, O_NONBLOCK) == -1)
        {
            osal_debug_error("osal_event_pipefd: pipe2() failed");
            return -1;
        }
        if (write(pe->pipefd[1], "\n", 1) != 1)
        {
            osal_debug_error("pipe2 write failed");
        }

        printf ("OK\n");
    }


    return pe->pipefd[0];
}


/**
****************************************************************************************************

  @brief Clear data buffered in pipe.
  @anchor osal_event_clearpipe

  The osal_event_clearpipe() function..

  @param   evnt Event pointer returned by osal_event_create() function.
  @return  None.

****************************************************************************************************
*/
void osal_event_clearpipe(
    osalEvent evnt)
{
    osalPosixEvent *pe;
    char c;

    if (evnt == OS_NULL)
    {
        osal_debug_error("osal_event_pipefd: NULL argument");
        return;
    }

    /* Cast posix event pointer.
     */
    pe = (osalPosixEvent*)evnt;

    if (pe->pipefd[0] != -1)
    {
        while (read(pe->pipefd[0], &c, 1) > 0);
    }
    pe->signaled = OS_FALSE;
}

#endif
