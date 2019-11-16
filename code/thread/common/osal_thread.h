/**

  @file    thread/common/osal_thread.h
  @brief   Creating, terminating, scheduling and identifying threads.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    29.5.2016

  This header file contains functions prototypes and definitions for creating, terminating,
  scheduling and identifying threads of execution.
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
#ifndef OSAL_THREAD_INCLUDED
#define OSAL_THREAD_INCLUDED

/** 
****************************************************************************************************

  @name Thread Priorities.

  The osal_thread_set_priority() function sets current thread priority. Thread priority will
  set how operating system scheduler should share time between threads.
  Most systems we work on implement preemptive multitasking. This mean that threads running
  on higher priority get processor time first, and only when no higher priority thread needs
  time then the lower priority threads will receive it. Multiprocessor or multi-core environment
  will change this, since one processor or core can be executing only one task.

  The OSAL_THREAD_PRIORITY_LOW, OSAL_THREAD_PRIORITY_NORMAL and OSAL_THREAD_PRIORITY_HIGH are .
  used to prioritize execution of normal threads. The OSAL_THREAD_PRIORITY_TIME_CRITICAL is
  reserved for real time tasks only, and using this priority will put special requirements on 
  the thread.

****************************************************************************************************
*/
/*@{*/

/** Enumeration of thread priorities. These are priority values given as argument to
    osal_thread_set_priority() function. Use OSAL_THREAD_PRIORITY_TIME_CRITICAL with caution,
    since processor load of threads running at this priority may not exceeed processor
    capacity at any time.
    @anchor osalThreadPriority
 */
typedef enum
{
    /** Low priority thread. The low priority should be used for non time critical threads,
        which should run on background or are not as urgent as most of other threads.
	 */
    OSAL_THREAD_PRIORITY_LOW = 10,

	/** Normal priority thread. Normal priority should be used for most of threads.
	 */
    OSAL_THREAD_PRIORITY_NORMAL = 20,

	/** High priority thread. High priority should be used for threads which are important,
        but no absolute real time quarantee is needed.
	 */
    OSAL_THREAD_PRIORITY_HIGH = 30,

	/** Real time thread. The highest normal priority, used only for threads which must always
	    run in real time. Use this priority only with caution: Total processor load of all real 
		time threads may never exceed processor capacity at any time, otherwise deterministic
		behavior of system is broken. 
	 */
    OSAL_THREAD_PRIORITY_TIME_CRITICAL = 40
}
osalThreadPriority;

/*@}*/


/** 
****************************************************************************************************

  @name Thread Entry Point Function 

  When a new thread is created by osal_thread_create() function, pointer to user defined thread
  entry point function is given as argument. The user defined entry point function must be a
  function with no return value, and taking two arguments. Void pointer to parameters (prm),
  which is typically pointer to user defined parameter structure for the new thread. Second
  argument is done event, which the new thread must set by calling osal_event_set() function
  once the new thread has processed parameters and no longer will refer to prm pointer.

****************************************************************************************************
*/
/*@{*/

/** Thread entry point function type. Pointer to user defined thread entry point function
    is given as argument to osal_thread_create() function to start a new thread. The thread
    entry point function needs to copy parameters to local stack, etc. and then set event
	"done", so that thread which called this function can proceed.
    @anchor osal_thread_func

    @param  prm Pointer to parameters for new thread. This pointer must can be used only
            before "done" event is set. This can be OS_NULL if no parameters are needed.
	@param  done Event to set when parameters have been copied to thread's own memory and
	        prm pointer is no longer needed by thread function. 
	@return None.
 */
typedef void osal_thread_func(
	void *prm,
    osalEvent done);

/*@}*/


/**
****************************************************************************************************

  @name Thread handle type

  Thread handle returned by osal_thread_create can be used to join worker thread to thread
  which created it, or to request interrupting a thread. Detached threads to not have 
  thread handle and cannot be joined or interrupted.

  The thread handle structure is abstract structure, which is defined only for compiler type 
  checking. Actual thread handle content depends on operating system, etc.

****************************************************************************************************
*/
typedef struct 
{
	/** Dummy member variable, never used.
	 */
	os_int dummy;
} 
osalThreadHandle;


/**
****************************************************************************************************

  @name Flags for creating thread

  Flags OSAL_THREAD_ATTACHED or OSAL_THREAD_DETACHED given to osal_thread_create sets is
  the newly created thread is to be attached to the thread which created this one.
  If flag OSAL_THREAD_ATTACHED is given, the new thread is attached to calling thread 
  and must eventually be joined back to it by osal_thread_join() function. In this case
  the osal_thread_create() returns thread handle which is used as argument to join and
  can be used to to request worker thread to exit by osal_thread_request_exit() call.
  If OSAL_THREAD_DETACHED is given, newly created thread is detached from thread which
  created it. The osal_thread_create() returns OS_NULL and join or request exit functions
  are not available.

****************************************************************************************************
*/
/*@{*/
	#define OSAL_THREAD_ATTACHED 1
	#define OSAL_THREAD_DETACHED 2
/*@}*/


/** 
****************************************************************************************************

  @name Thread Functions

  A new thread is created by osal_thread_create(). Functions osal_thread_set_priority() and
  os_sleep() are used to set scheduling and control timing of the thread execution.
  The osal_thread_get_id() function retrieves integer number uniquely identifying the thread.

****************************************************************************************************
 */
/*@{*/
#if OSAL_MULTITHREAD_SUPPORT
    /* Create a new thread.
     */
    osalThreadHandle *osal_thread_create(
        osal_thread_func *func,
        void *prm,
	    os_int flags,
        os_memsz stack_size,
        const os_char *name);

    /* Join worker thread to this thread (one which created the worker)
       Releases thread handle.
     */
    void osal_thread_join(
	    osalThreadHandle *handle);

    /* Set thread priority.
     */
    osalStatus osal_thread_set_priority(
	    osalThreadPriority priority);

    /* Get thread ID.
     */
    os_long osal_thread_get_id(
        os_int reserved);

    /* Sleep until end of time slice.
     */
    void os_timeslice(void);

#else
    /* Macros to do nothing if OSAL_MULTITHREAD_SUPPORT is not selected.
     */
    #define osal_thread_create(f,p,l,s,n) OS_NULL
    #define osal_thread_join(h)
    #define osal_thread_set_priority(p) OSAL_SUCCESS
    #define osal_thread_get_id(r) 0
    #define os_timeslice()

#endif

    /* Suspend thread execution for a specific time, milliseconds.
     */
    void os_sleep(
        os_long time_ms);

    /* Suspend thread execution for a specific time, microseconds.
     */
    void os_microsleep(
        os_long time_us);

/*@}*/


#endif
