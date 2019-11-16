/**

  @file    mutex/windows/osal_mutex.c
  @brief   Mutexes, synchronizing thread access to shared resources.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This file implements mutex related functionality for Windows. Generally mutexes are used to
  synchronize thread access of shared resources. A mutex is created by osal_mutex_create()
  function and deleted by osal_mutex_delete() function. Before accessing a shared resource
  a thread must call osal_mutex_lock(), and osal_mutex_unlock() once it finishes access 
  to the shared resource. This will cause other threads calling osal_mutex_lock() to block
  until the first thread has finished with the shared resource. An OSAL mutex is always so 
  called recursive mutex.

  System mutex is a global mutex for whole process used to synchronize access to global 
  variables, etc. When system mutex is locked by os_lock() function, the 
  thread is switched to very high priority to prevent priority reversal. The old thread priority
  is restored when system mutex is unlocked by os_unlock() function. Using
  one system mutex (as far as reasonable) instead of many mutexes will prevent deadlock
  situations. Limitation is that System mutex lock time should be minimized to be as short 
  as possible, since system mutex locks may halt many threads. For example never lock system
  mutex when waiting for IO, etc.

  Define OSAL_MULTITHREAD_SUPPORT controls compiler's code generation for supporting mutexes. 
  If the define is nonzero, then code for mutex support is generated, otherwise not.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#define _WIN32_WINNT 0x0403
#include <windows.h>

#if OSAL_MULTITHREAD_SUPPORT

#if OSAL_DEBUG
static os_char osal_mutex_null_ptr_msg[] = "NULL mutex pointer";
#endif

/**
****************************************************************************************************

  @brief Initialize OSAL mutexes.
  @anchor osal_mutex_initialize

  The osal_mutex_initialize() function initializes OSAL mutex support and creates system mutex. 
  This function is called by osal_initialize() and should not normally be called by application.
  Windows: Critical section objects are used to implement mutexes. No initialization is
  needed. 

  @return  None.

****************************************************************************************************
*/
void osal_mutex_initialize(
    void)
{
	/* Create system mutex.
	 */
	osal_global->system_mutex = osal_mutex_create();
}


/**
****************************************************************************************************

  @brief Shut down OSAL mutexes.
  @anchor osal_mutex_shutdown

  The osal_mutex_shutdown() function releases system mutex and shuts down OSAL mutex support. 
  This function is called by osal_shutdown() and should not normally be called by application.
  Code for this function is generated only if OSAL_PROCESS_CLEANUP_SUPPORT define is nonzero.
  Windows: No shut down code is requires, just deletes system mutes.

  @return  None.

****************************************************************************************************
*/
#if OSAL_PROCESS_CLEANUP_SUPPORT
void osal_mutex_shutdown(
    void)
{
	/* Delete system mutex.
	 */
	osal_mutex_delete(osal_global->system_mutex);
}
#endif


/**
****************************************************************************************************

  @brief Create a mutex.
  @anchor osal_mutex_create

  The osal_mutex_create() function creates an mutex. Mutexes are used to synchronize thread 
  access of shared resources. Mutex created by this function can be deleted by osal_mutex_delete()
  function. Before accessing a shared resource a thread must call osal_mutex_lock(), and 
  osal_mutex_unlock() once it finishes access to the shared resource. This will cause other 
  threads calling osal_mutex_lock() on same mutex to block until the first thread has finished 
  with the shared resource. An OSAL mutex is always so called recursive mutex.
  Resource monitor's mutex count is incremented, if resource monitor is enabled.

  @return  mutex Mutex pointer. If the function fails, it returns OS_NULL.

****************************************************************************************************
*/
osalMutex osal_mutex_create(
    void)
{
    LPCRITICAL_SECTION
        critical_section;

	/* Allocate memory for critical section. Here we cannot use os_malloc(), since
	   mutexes are initialized before memory (memory allocation needs mutexes).
	 */
    critical_section = HeapAlloc(GetProcessHeap(), 0, (DWORD)sizeof(CRITICAL_SECTION));
    if (critical_section == NULL) return OS_NULL;

    /* Call Windows to initialize critical section.
     */
	InitializeCriticalSection(critical_section);

    /* Inform resource monitor that new mutex has been succesfullly created.
     */
    osal_resource_monitor_increment(OSAL_RMON_MUTEX_COUNT);

    /* Return the mutex pointer.
     */
    return (osalMutex)critical_section;
}


/**
****************************************************************************************************

  @brief Delete a mutex.
  @anchor osal_mutex_delete

  The osal_mutex_delete() function deletes an mutex which was created by osal_mutex_create()
  function. Resource monitor's mutex count is decremented, if resource monitor is enabled.

  @param   mutex Pointer to mutex to delete. 

  @return  None.

****************************************************************************************************
*/
void osal_mutex_delete(
    osalMutex mutex)
{
    if (mutex == OS_NULL)
    {
        osal_debug_error(osal_mutex_null_ptr_msg);
        return;
    }

	DeleteCriticalSection((LPCRITICAL_SECTION)mutex);

    /* Release memory.
     */
    HeapFree(GetProcessHeap(), 0, mutex);

    /* Inform resource monitor that mutex has been deleted.
     */
    osal_resource_monitor_decrement(OSAL_RMON_MUTEX_COUNT);
}


/**
****************************************************************************************************

  @brief Lock a mutex.
  @anchor osal_mutex_lock

  The osal_mutex_lock() function locks the mutex (increments mutex'es lock count). If an another
  thread tries to lock the mutex while it is locked, it is suspended until the thread which
  originally locked the mutex releases it. All OSAL mutexes are recursive mutexes, thus if a 
  thread calls os_lock() function recursively, it will not block. Just lock count
  is incremented.

  @param   mutex Mutex pointer returned by osal_mutex_create() function.
  @return  None.

****************************************************************************************************
*/
void osal_mutex_lock(
    osalMutex mutex)
{
    if (mutex == OS_NULL)
    {
        osal_debug_error(osal_mutex_null_ptr_msg);
        return;
    }

	EnterCriticalSection((LPCRITICAL_SECTION)mutex);
}


/**
****************************************************************************************************

  @brief Release a mutex.
  @anchor osal_mutex_unlock

  The osal_mutex_unlock() function decrements mutexes lock count. If the resulting lock count
  is zero, the mutex is released and a suspended thread wating in  There must be exactly one
  osal_mutex_unlock() function call for each osal_mutex_lock() function call.

  @param   mutex Mutex pointer returned by osal_mutex_create() function.

  @return  None.

****************************************************************************************************
*/
void osal_mutex_unlock(
    osalMutex mutex)
{
    if (mutex == OS_NULL)
    {
        osal_debug_error(osal_mutex_null_ptr_msg);
        return;
    }

	LeaveCriticalSection((LPCRITICAL_SECTION)mutex);
}


/**
****************************************************************************************************

  @brief Lock the system mutex.
  @anchor os_lock

  The os_lock() function switches the thread to very high priority to prevent 
  priority reversal and locks system mutex. The function saves old priority to be restored 
  once the system mutex is unlocked by os_unlock() function. System mutex
  is recursive mutex, like all OSAL mutexes, thus if a thread calls os_lock()
  function recursively, it will not block. Just lock count is incremented.

  System mutex is a global mutex for whole process used to synchronize access to global 
  variables, etc. Using the one system mutex (as far as reasonable) instead of many mutexes 
  will prevent deadlock situations. Anyhow the system mutex lock time should be minimized 
  to be as short  as possible, since system mutex lock may halt many threads. For example 
  never lock system mutex when waiting for IO, etc.

  @return  None.

****************************************************************************************************
*/
void os_lock(
    void)
{
#if OSAL_TIME_CRITICAL_SYSTEM_LOCK
	HANDLE 
		handle;

	int 
        winpriority;

	/* Save the current priority and switch to very high priority. This is done to prevent 
	   any change of priority reversal. Notice that osal_global structure must not be modified
	   before the synchronization starts.
	 */
	handle = GetCurrentThread();
	winpriority = GetThreadPriority(handle);
    SetThreadPriority(handle, THREAD_PRIORITY_TIME_CRITICAL);

	/* Start synchronization.
	 */
	EnterCriticalSection((LPCRITICAL_SECTION)osal_global->system_mutex);

	/* Increment system mutex lock count. If this is not recursive call, save priority.
	 */
	if (osal_global->system_mutex_lock_count++ == 0)
	{
		osal_global->system_mutex_enter_priority = winpriority;
		osal_global->system_mutex_thread = handle;
	}
#else
	/* Start synchronization.
	 */
	EnterCriticalSection((LPCRITICAL_SECTION)osal_global->system_mutex);
#endif
}


/**
****************************************************************************************************

  @brief Release the system mutex.
  @anchor os_unlock

  The os_unlock() function unlocks the system mutex which was locked by the
  os_lock() function and restores thread priority, which was used before 
  locking the system mutex. 

  @return  None.

****************************************************************************************************
*/
void os_unlock(
    void)
{
#if OSAL_TIME_CRITICAL_SYSTEM_LOCK
	HANDLE
		handle;

	int 
        winpriority;

	os_int
		system_mutex_lock_count;

	/* Decrement system mutex lock count, and save the lock count value. Copy saved 
	   priority and thread handle from global structure to local variable. This must 
	   be done before synchronization ends.
	 */
	system_mutex_lock_count = --(osal_global->system_mutex_lock_count);
	winpriority = osal_global->system_mutex_enter_priority;
	handle = osal_global->system_mutex_thread;

	/* End synchronization.
	 */
	LeaveCriticalSection((LPCRITICAL_SECTION)osal_global->system_mutex);

    /* If the system mutex was unlocked (no recursion), then restore the saved priority.
     */
    if (system_mutex_lock_count == 0) 
    {
        SetThreadPriority(handle, winpriority);
    }
#else
	/* End synchronization.
	 */
	LeaveCriticalSection((LPCRITICAL_SECTION)osal_global->system_mutex);
#endif
}

#endif
