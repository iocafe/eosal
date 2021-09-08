/**

  @file    thread/common/osal_thread.h
  @brief   Creating, terminating, scheduling and identifying threads.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  This header file contains defines and function prototypes for eosal threads API.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_THREAD_H_
#define OSAL_THREAD_H_
#include "eosal.h"

/**
****************************************************************************************************

  @brief Thread Priorities.
  @anchor osalThreadPriority

  Enumeration of thread priorities. These are priority values given as argument to
  osal_thread_set_priority() function. Use OSAL_THREAD_PRIORITY_TIME_CRITICAL with caution,
  since processor load of threads running at this priority may not exceeed processor
  capacity at any time.

****************************************************************************************************
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


/**
****************************************************************************************************

  @brief Thread entry point function
  @anchor osal_thread_func

  Thread entry point function type. Pointer to user defined thread entry point function
  is given as argument to osal_thread_create() function to start a new thread. The thread
  entry point function needs to copy parameters to local stack, etc. and then set event
  "done", so that thread which called this function can proceed.

  @param  prm Pointer to parameters for new thread. This pointer must can be used only
          before "done" event is set. This can be OS_NULL if no parameters are needed.
  @param  done Event to set when parameters have been copied to thread's own memory and
          prm pointer is no longer needed by thread function.

****************************************************************************************************
*/
typedef void osal_thread_func(
    void *prm,
    osalEvent done);


/**
****************************************************************************************************

  @brief Thread handle type

  When an attached thread is created, ahread handle is returned by osal_thread_create
  function. It needs to be used to join worker thread back which created it. Detached threads
  do not have thread handle and cannot be joined or interrupted.

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
osalThread;


/**
****************************************************************************************************

  @name Flags for creating thread

  Flags OSAL_THREAD_ATTACHED or OSAL_THREAD_DETACHED given to osal_thread_create sets if
  the newly created thread is to be attached to the thread which created this one.
  If flag OSAL_THREAD_ATTACHED is given, the new thread is attached and must eventually
  be joined back to it by osal_thread_join() function. In this case the osal_thread_create()
  returns thread handle which is used as argument to join.
  If OSAL_THREAD_DETACHED is given, newly created thread is detached from thread which
  created it. The osal_thread_create() returns OS_NULL.

****************************************************************************************************
*/
#define OSAL_THREAD_ATTACHED 1
#define OSAL_THREAD_DETACHED 2


/**
****************************************************************************************************

  @name Optional thread parameters

  This parameter structure can be given when creating a new thread. It contains opetional
  and some platform dependent settings for a new thread. Allocate this structure from stack,
  use os_memclear to fill it with zeros and set only parameters you want to modify from defaults.

****************************************************************************************************
 */
typedef struct osalThreadOptParams
{
    /** Name for the new thread. Some operating systems allow naming threads, which is very
        useful for debugging. If no name is needed this can be NULL.
     */
    const os_char *thread_name;

    /** Stack size for the new thread in bytes. Value 0 creates thread with default stack
        size for operating system.
     */
    os_memsz stack_size;

    /** Priority for the new thread, for example OSAL_THREAD_PRIORITY_NORMAL. If zero, default
       is used.
     */
    osalThreadPriority priority;

    /** Pin thread to specific processor core.
     */
    os_boolean pin_to_core;

    /** Core number to pin to if pin_to_core is set.
     */
    os_short pin_to_core_nr;
}
osalThreadOptParams;


/**
****************************************************************************************************

  @name Default task stack sizes in bytes.

  These can be overridden for operating system or for a spacific build.

****************************************************************************************************
 */
#ifndef OSAL_THREAD_SMALL_STACK
#define OSAL_THREAD_SMALL_STACK 4096
#endif
#ifndef OSAL_THREAD_NORMAL_STACK
#define OSAL_THREAD_NORMAL_STACK 8192
#endif
#ifndef OSAL_THREAD_LARGE_STACK
#define OSAL_THREAD_LARGE_STACK 16384
#endif


#if OSAL_MULTITHREAD_SUPPORT
/**
****************************************************************************************************

  @name Thread Functions

  Functions for creating, joining and priorizing threads.

****************************************************************************************************
 */
/* Create a new thread.
 */
osalThread *osal_thread_create(
    osal_thread_func *func,
    void *prm,
    osalThreadOptParams *opt,
    os_int flags);

/* Join worker thread to this thread (one which created the worker)
   Releases thread handle.
 */
void osal_thread_join(
    osalThread *handle);

/* Set thread priority.
 */
osalStatus osal_thread_set_priority(
    osalThreadPriority priority);

/* Sleep until end of time slice.
 */
void os_timeslice(void);

/* Convert OSAL thread priority to underlyin OS priority number.
 */
os_int osal_thread_priority_to_sys_priority(
    osalThreadPriority priority);


#else
/**
****************************************************************************************************

  @name Empty macros when multithreading is not used.

  Macros to do nothing if OSAL_MULTITHREAD_SUPPORT is not selected. These allow to compile code
  with multithreading calls for single thread environment.

****************************************************************************************************
 */
    #define osal_thread_create(f,p,o,l) OS_NULL
    #define osal_thread_join(h)
    #define osal_thread_set_priority(p) OSAL_SUCCESS
    #define os_timeslice()

#endif


/**
****************************************************************************************************

  @name Sleep functions

  These suspend execution of a thread for specific period.

****************************************************************************************************
 */
/* Suspend thread execution for a specific time, milliseconds.
 */
void osal_sleep(
    os_long time_ms);

/* Suspend thread execution for a specific time, microseconds.
 */
void os_microsleep(
    os_long time_us);


#endif
