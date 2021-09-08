/**

  @file    thread/windows/osal_windows_thread.c
  @brief   Creating, terminating, scheduling and identifying threads.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Thread functions for Windows are implemented here.
  A process can run multiple tasks concurrently, and these concurrently running tasks are called
  threads of execution. The treads of the same process and share memory and other processe's
  resources. Typically access to shared resources must be synchronized, see mutexes.
  On a single processor the processor switches between different threads. This context switching
  generally happens frequently enough that the user perceives the threads or tasks as running
  at the same time. On a multiprocessor or multi-core system, the threads or tasks will generally
  run at the same time, with each processor or core running a particular thread or task.
  A new thread is created by osal_thread_create() function call
  Thread priorizing is handled by osal_thread_set_priority() function.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#ifdef OSAL_WINDOWS
#if OSAL_MULTITHREAD_SUPPORT

/** Intermediate parameter structure when creating a new Windows thread.
 */
typedef struct
{
    /** Pointer to thread entry point function.
     */
    osal_thread_func *func;

    /** Parameters to pass to the new thread.
     */
    void *prm;

    /** Flags argument of osal_thread_create().
     */
    os_int flags;

    /** Priority for the new thread.
     */
    osalThreadPriority priority;

    /** Event to set when parameters have been copied to entry point functions own memory.
     */
    osalEvent done;
}
osalWindowsThreadPrms;

/** Operating system specific thread handle.
*/
typedef struct
{
    /* Windows thread handle.
    */
    HANDLE thread_handle;
}
osalWindowsThreadHandle;

/* Forward referred static functions.
 */
static DWORD WINAPI osal_thread_intermediate_func(
    LPVOID lpParameter);


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
  created by this function must be eventually either terminated by returning from thread
  function. All new threads start with normal priority OSAL_THREAD_PRIORITY_NORMAL, but the
  entry point function can call osal_thread_set_priority() to set it's own priority.

  @param   func Pointer to thread entry point function. See osal_thread_func() for entry point
           function type.
  @param   prm Pointer to parameters (typically pointer to used defined structure) to pass to
           thread entry point function. This can be allocated from local stack, etc.
           The thread entry point function is responsible of copying this stucture to it's
           own memory, and then calling osal_event_set() to set done event to allow the
           calling thread to proceed. If no parameters are needed, this can be OS_NULL,
           but even then entry point function must set done event.
  @param   opt Pointer to thread options structure, like thread name, stack size, etc.  Can
           be set to OS_NULL to used defaults.
  @param   Flags OSAL_THREAD_ATTACHED or OSAL_THREAD_DETACHED given to osal_thread_create sets is
           the newly created thread is to be attached to the thread which created this one.
           If flag OSAL_THREAD_ATTACHED is given, the new thread is attached to calling thread
           and must eventually be joined back to it by osal_thread_join() function. In this case
           the osal_thread_create() returns thread handle which is used as argument to join and
           can be used to to request worker thread to exit by osal_thread_request_exit() call.
           If OSAL_THREAD_DETACHED is given, newly created thread is detached from thread which
           created it. The osal_thread_create() returns OS_NULL and join or request exit functions
           are not available.

  @return  Pointer to thread handle if OSAL_THREAD_ATTACHED flags is given, or OS_NULL otherwise.

****************************************************************************************************
*/
osalThread *osal_thread_create(
    osal_thread_func *func,
    void *prm,
    osalThreadOptParams *opt,
    os_int flags)
{
    HANDLE
        thread_handle;

    DWORD
        thread_id;

    osalWindowsThreadPrms
        winprm;

    osalWindowsThreadHandle
        *handle;

    SIZE_T
         stack_size = 0; /* 0 = use Windows default for .exe */

    /* Save pointers to thread entry point function and to parameters into
       thread creation parameter structure.
     */
    os_memclear(&winprm, sizeof(osalWindowsThreadPrms));
    winprm.func = func;
    winprm.prm = prm;
    winprm.flags = flags;

    /* Increment thread count, for "process ready to exit". Order of checking
     * exit_process flag and modifying thread_count is significant.
     */
    osal_global->thread_count++;
    if (osal_global->exit_process) {
        osal_global->thread_count--;
        return OS_NULL;
    }

    /* Create event to wait until newly created thread has processed it's parameters. If creating
       the event fails, return the error code.
     */
    winprm.done = osal_event_create(OSAL_EVENT_DEFAULT);
    if (winprm.done == OS_NULL)
    {
        osal_debug_error("osal_thread,osal_event_create failed");
        osal_global->thread_count--;
        return OS_NULL;
    }

    if (flags & OSAL_THREAD_ATTACHED)
    {
        handle = (osalWindowsThreadHandle*)osal_sysmem_alloc(sizeof(osalWindowsThreadHandle), OS_NULL);
        os_memclear(handle, sizeof(osalWindowsThreadHandle));
    }
    else
    {
        handle = OS_NULL;
    }

    /* Process options, if any.
     */
    winprm.priority = OSAL_THREAD_PRIORITY_NORMAL;
    if (opt)
    {
        if (opt->priority) winprm.priority = opt->priority;
        if (opt->stack_size) stack_size = (SIZE_T)opt->stack_size;
    }

    /* Call Windows to create and start the new thread.
     */
    thread_handle = CreateThread(NULL, stack_size, osal_thread_intermediate_func,
        &winprm, 0, &thread_id);

    /* If thread creation fails, then return error code.
     */
    if (thread_handle == NULL)
    {
        osal_debug_error("osal_thread,CreateThread failed");
        osal_sysmem_free(handle, sizeof(osalWindowsThreadHandle));
        osal_event_delete(winprm.done);
        osal_global->thread_count--;
        return OS_NULL;
    }

    /* Inform resource monitor that thread has been successfully created.
     */
    osal_resource_monitor_increment(OSAL_RMON_THREAD_COUNT);

    /* Wait for "done" event. This is set once the new thread has taken over the parameters.
     */
    osal_event_wait(winprm.done, OSAL_EVENT_INFINITE);

    /* Delete the event.
     */
    osal_event_delete(winprm.done);

    /* If we attach the new thread, save windows thread handle. Otherwise close it.
     */
    if (handle)
    {
        handle->thread_handle = thread_handle;
    }
    else
    {
        /* Close the Windows thread handle so that no zombie handles are left hanging around.
         */
        CloseHandle(thread_handle);
    }

    /* Success.
     */
    return (osalThread*)handle;
}


/**
****************************************************************************************************

  @brief Intermediate thread entry point function.

  The osal_thread_intermediate_func() function is called to start executing code under
  newly created thread. It calls application's osal_thread_func() trough function pointer.

  @param   lpParameter Pointer to parameter structure for new Windows thread.
  @return  Always 1.

****************************************************************************************************
*/
static DWORD WINAPI osal_thread_intermediate_func(
  LPVOID lpParameter)
{
    osalWindowsThreadPrms *winprm;
    os_int flags;

    /* Cast the pointer
     */
    winprm = (osalWindowsThreadPrms*)lpParameter;
    flags = winprm->flags;

    /* Use specified thread priotity.
     */
    osal_thread_set_priority(winprm->priority);

    /* Call the final thread entry point function.
     */
    winprm->func(winprm->prm, winprm->done);

    /* Inform resource monitor that thread is terminated.
    */
    osal_resource_monitor_decrement(OSAL_RMON_THREAD_COUNT);

    /* If this is detached thread, decrement thread count.
     */
    if (flags & OSAL_THREAD_DETACHED) {
        osal_global->thread_count--;
    }

    /* Return success.
     */
    return 1;
}


/**
****************************************************************************************************

  @brief Join attached worker thread
  @anchor osal_thread_join

  The osal_thread_join() function must be called for threads created by osal_thread_create()
  with OSAL_THREAD_ATTACHED flag. The function waits until worker thread exists and frees the
  handle and associated resources.

  Notice that this function doesn't signal worker thread to exit.

  @param   handle Thread handle as returned by osal_thread_create.

****************************************************************************************************
*/
void osal_thread_join(
    osalThread *handle)
{
    /* Check for programming errors.
     */
    if (handle == OS_NULL)
    {
        osal_debug_error("osal_thread,osal_thread_join: NULL handle");
        return;
    }

    /* Join the thread.
     */
    WaitForSingleObject(((osalWindowsThreadHandle*)handle)->thread_handle, INFINITE);

    /* Delete the handle structure.
     */
    CloseHandle(((osalWindowsThreadHandle*)handle)->thread_handle);
    osal_sysmem_free(handle, sizeof(osalWindowsThreadHandle));

    /* Decrement thread count.
     */
    osal_global->thread_count--;
}


/**
****************************************************************************************************

  @brief Suspend thread execution until end of time slice.
  @anchor osal_sleep

  The os_timeslice() function suspends the execution of the current thread for a very short
  period, typically until end of processor time slice. If multi-threading is not supported,
  this function does nothing.

  @return  None.

****************************************************************************************************
*/
void os_timeslice(void)
{
    Sleep(2);
}

#endif
#endif