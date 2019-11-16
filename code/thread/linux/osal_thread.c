/**

  @file    thread/linux/osal_thread.c
  @brief   Creating, terminating, scheduling and identifying threads.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Thread functions for Linux.
  A process can run multiple tasks concurrently, and these concurrently running tasks are called
  threads of execution. The treads of the same process and share memory and other processe's
  resources. Typically access to shared resources must be synchronized, see mutexes.
  On a single processor the processor switches between different threads. This context switching
  generally happens frequently enough that the user perceives the threads or tasks as running
  at the same time. On a multiprocessor or multi-core system, the threads or tasks will generally
  run at the same time, with each processor or core running a particular thread or task.
  A new thread is created by osal_thread_create() function call
  Thread priorizing and sleep are handled by osal_thread_set_priority() and
  os_sleep() functions. Threads of execution can be identified by osal_thread_get_id()
  function.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#if OSAL_MULTITHREAD_SUPPORT

#include <sched.h>
#include <pthread.h>
#include <unistd.h>


/** Intermediate parameter structure when creating a new Linux thread.
 */
typedef struct
{
    /** Pointer to thread entry point function.
     */
    osal_thread_func *func;

    /** Parameters to pass to the new thread.
     */
    void *prm;

    /** Event to set when parameters have been copied to entry point functions own memory.
     */
    osalEvent done;
}
osalLinuxThreadPrms;

/** Operating system specific thread handle.
*/
typedef struct
{
    /* Linux thread handle.
	*/
    pthread_t threadh;
}
osalLinuxThreadHandle;
#endif

/* Forward referred static functions.
 */
#if OSAL_MULTITHREAD_SUPPORT
static void *osal_thread_intermediate_func(
  void *parameters);
#endif


#if OSAL_MULTITHREAD_SUPPORT
/**
****************************************************************************************************

  @brief Create a new thread.
  @anchor osal_thread_create

  The osal_thread_create() function creates and starts a new thread of execution. When a new
  thread is created by osal_thread_create function, pointer to user defined thread entry
  point function is given as argument. The user defined entry point function must be a function
  with no return value, and taking two arguments. Void pointer to parameters (prm), which is
  typically pointer to user defined parameter structure for the new thread. Second argument is
  done event, which the new thread must set by calling osal_event_set() function once the new
  thread has processed parameters and no longer will refer to prm pointer. Every thread
  created by this function must be eventually either terminated by explicit osal_exit_thread()
  function call, or by returning from entry point function which will result osal_exit_thread()
  call. All new threads start with normal priority OSAL_THREAD_PRIORITY_NORMAL, but the
  entry point function can call osal_thread_set_priority() to set it's own priority.
  
  @param   func Pointer to thread entry point function. See osal_thread_func() for entry point
           function type.
  @param   prm Pointer to parameters (typically pointer to used defined structure) to pass to
           thread entry point function. This can be allocated from local stack, etc.
           The thread entry point function is responsible of copying this stucture to it's
           own memory, and then calling osal_event_set() to set done event to allow the
           calling thread to proceed. If no parameters are needed, this can be OS_NULL,
           but even then entry point function must set done event.
  @param   Flags OSAL_THREAD_ATTACHED or OSAL_THREAD_DETACHED given to osal_thread_create sets is
		   the newly created thread is to be attached to the thread which created this one.
		   If flag OSAL_THREAD_ATTACHED is given, the new thread is attached to calling thread
		   and must eventually be joined back to it by osal_thread_join() function. In this case
		   the osal_thread_create() returns thread handle which is used as argument to join and
		   can be used to to request worker thread to exit by osal_thread_request_exit() call.
		   If OSAL_THREAD_DETACHED is given, newly created thread is detached from thread which
		   created it. The osal_thread_create() returns OS_NULL and join or request exit functions
		   are not available.
  @param   stack_size Initial stack size in bytes for the new thread. Value 0 creates thread
           with default stack size for operating system.
  @param   name Name for the new thread. Some operating systems allow naming
           threads, which is very useful for debugging. If no name is needed this can be
           set to OS_NULL.

  @return  Pointer to thread handle if OSAL_THREAD_ATTACHED flags is given, or OS_NULL otherwise.

****************************************************************************************************
*/
osalThreadHandle *osal_thread_create(
	osal_thread_func *func,
	void *prm,
	os_int flags,
	os_memsz stack_size,
	const os_char *name)
{
    osalLinuxThreadPrms linprm;
    osalLinuxThreadHandle *handle;
    pthread_attr_t attrib;
    pthread_t threadh;
    int s;

    /* Save pointers to thread entry point function and to parameters into
       thread creation parameter structure.
     */
    os_memclear(&linprm, sizeof(osalLinuxThreadPrms));
    linprm.func = func;
    linprm.prm = prm;

    /* Create event to wait until newly created thread has processed it's parameters. If creating
       the event failes, return the error code.
     */
    linprm.done = osal_event_create();
    if (linprm.done == OS_NULL)
    {
		osal_debug_error("osal_thread,osal_event_create failed");
        return OS_NULL;
    }

	if (flags & OSAL_THREAD_ATTACHED)
	{
        handle = (osalLinuxThreadHandle*)os_malloc(sizeof(osalLinuxThreadHandle), OS_NULL);
        os_memclear(handle, sizeof(osalLinuxThreadHandle));
	}
	else
	{
		handle = OS_NULL;
	}

    pthread_attr_init(&attrib);
    pthread_attr_setschedpolicy(&attrib, SCHED_OTHER);
    pthread_attr_setdetachstate(&attrib,
        handle ? PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED);

    // pthread_attr_setstacksize(&attrib, stack_size);

    /* Call linux to create and start the new thread.
     */
    s = pthread_create(&threadh, &attrib,
        osal_thread_intermediate_func, &linprm);
    pthread_attr_destroy(&attrib);

    /* If thread creation fails, then return error code.
     */
    if (s)
    {
        osal_debug_error("osal_thread,pthread_create failed");
        os_free(handle, sizeof(osalLinuxThreadHandle));
		return OS_NULL;
    }

    /* Inform resource monitor that thread has been succesfully creted.
     */
    osal_resource_monitor_increment(OSAL_RMON_THREAD_COUNT);

    /* Wait for "done" event. This is set once the new thread has taken over the parameters.
     */
    osal_event_wait(linprm.done, OSAL_EVENT_INFINITE);

    /* Delete the event.
     */
    osal_event_delete(linprm.done);

    /* If we created joinable thread, save linux thread handle.
	 */
	if (handle)
	{
        handle->threadh = threadh;
	}

    /* Success.
     */
    return (osalThreadHandle*)handle;
}
#endif


#if OSAL_MULTITHREAD_SUPPORT
/**
****************************************************************************************************

  @brief Intermediate thread entry point function.

  The osal_thread_intermediate_func() function is intermediate function to start a new thread.
  This function exists to make user's thread entry point function type same on all operating
  systems.

  @param   lpParameter Pointer to parameter structure to start Windows thread.
  @return  OS_NULL.

****************************************************************************************************
*/
static void *osal_thread_intermediate_func(
  void *parameters)
{
    osalLinuxThreadPrms
        *linprm;

    /* Make sure that we are running on normal thread priority.
     */
    osal_thread_set_priority(OSAL_THREAD_PRIORITY_NORMAL);

    /* Cast the pointer
     */
    linprm = (osalLinuxThreadPrms*)parameters;

    /* Call the final thread entry point function.
     */
    linprm->func(linprm->prm, linprm->done);

	/* Inform resource monitor that thread is terminated.
	*/
	osal_resource_monitor_decrement(OSAL_RMON_THREAD_COUNT);
	
    /* Return.
     */
    return OS_NULL;
}
#endif


#if OSAL_MULTITHREAD_SUPPORT
/**
****************************************************************************************************

  @brief Join worker thread to this thread (one which created the worker)
  @anchor osal_thread_join

  The osal_thread_join() function is called by the thread which created worker thread to join
  worker thread back. This function MUST be called for threads created with
  OSAL_THREAD_ATTACHED flag and cannot be called for ones created with OSAL_THREAD_DETACHED flag.
  It may be good idea to call osal_thread_request_exit() just before calling this function, to
  make sure that worker thread knows to close.

  @param   handle Thread handle as returned by osal_thread_create.
  @return  None.

****************************************************************************************************
*/
void osal_thread_join(
	osalThreadHandle *handle)
{
    osalLinuxThreadHandle *linuxhandle;
    void *res;
    int s;

	/* Check for programming errors.
	 */
	if (handle == OS_NULL)
	{
		osal_debug_error("osal_thread,osal_thread_join: NULL handle");
		return;
	}

    linuxhandle = (osalLinuxThreadHandle*)handle;
    s = pthread_join(linuxhandle->threadh, &res);
    if (s != 0)
    {
        osal_debug_error("osal_thread,osal_thread_join failed");
        return;
    }

    /* free(res); Free memory allocated by thread */

	/* Delete the handle structure.
	 */
    os_free(handle, sizeof(osalLinuxThreadHandle));
}
#endif


/**
****************************************************************************************************

  @brief Suspend thread execution for a specific time, milliseconds.
  @anchor os_sleep

  The os_sleep() function suspends the execution of the current thread for a specified
  interval. The function is used for both to create timed delays and to force scheduler to give
  processor time to lower priority threads. If time_ms is zero the function suspends execution
  of the thread until end of current processor time slice.

  @param   time_ms Time to sleep, milliseconds. Value 0 sleeps until end of current processor
           time slice.
  @return  None.

****************************************************************************************************
*/
void os_sleep(
    os_long time_ms)
{
    if (time_ms >= 1000000)
    {
        sleep(time_ms/1000);
    }
    else
    {
        usleep(time_ms*1000);
    }
}


/**
****************************************************************************************************

  @brief Suspend thread execution for a specific time, microseconds.
  @anchor os_microsleep

  The os_microsleep() function suspends the execution of the current thread for a specified
  interval. The function is used for both to create timed delays and to force scheduler to give
  processor time to lower priority threads. If time_ms is zero the function suspends execution
  of the thread until end of current processor time slice.

  Windows specific: The function support only one millisecond precision. 

  @param   time_us Time to sleep, microseconds. Value 0 sleeps until end of current processor
           time slice.
  @return  None.

****************************************************************************************************
*/
void os_microsleep(
    os_long time_us)
{
    if (time_us >= 1000000000)
    {
        os_sleep(time_us/1000);
    }
    else
    {
        usleep(time_us);
    }
}


#if OSAL_MULTITHREAD_SUPPORT
/**
****************************************************************************************************

  @brief Suspend thread execution until end of time slice.
  @anchor os_sleep

  The os_timeslice() function suspends the execution of the current thread for a very short
  period, typically until end of processor time slice. If multi-threading is not supported,
  this function does nothing.

  @return  None.

****************************************************************************************************
*/
void os_timeslice(void)
{
    usleep(2000);
}
#endif

