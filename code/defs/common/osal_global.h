/**

  @file    defs/common/osal_global.h
  @brief   Global OSAL state structure.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This header file contains definition of OSAL global state structure.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

#if OSAL_MULTITHREAD_SUPPORT
    struct osalMutexStruct;
#endif

#if OSAL_FUNCTION_POINTER_SUPPORT
/* Extension module shutdown function type.
 */
typedef void osal_shutdown_func(void);
#endif


/** 
****************************************************************************************************

  @name Global Structure
  @anchor osalGlobalStruct

  The global structure saves OSAL library state. In practise this means all internal variables
  of the OSAL library. The variables are stored within a structure and the structure is always
  referred through osal_global pointer. Thus DLLs can share the same global structure with the 
  process which loaded the DLL. This will require only setting osal_global pointer in DLL.

****************************************************************************************************
*/
typedef struct
{
    /** OSAL library initialized flag.
     */
    os_boolean osal_initialized;

    /** Set to OS_TRUE to request "terminate the process".
     */
    os_boolean exit_process;

#if OSAL_MULTITHREAD_SUPPORT
	/** System mutex. System mutex is used to synchronize access to global variables, etc.
	 */
	struct osalMutexStruct *system_mutex;

#if OSAL_TIME_CRITICAL_SYSTEM_LOCK
	/** System mutex lock count. This is incremented by os_lock() function
	    and decremented by os_unlock() function.
	 */
	os_int system_mutex_lock_count;

	/** Saved priority when system mutex was locked. This is set by os_lock() 
	    function and restored by os_unlock() function. The value is 
		operating system's priority, not OSAL priority.
	 */
	os_int system_mutex_enter_priority;

	/** Saved system mutex thread handle. This is saved by os_lock() 
	    function and used by os_unlock() function.
	 */
	void *system_mutex_thread;
#endif
#endif

    /* Memory allocation related, see osal_memory.c
     */
#if OSAL_MEMORY_MANAGER

    /** Pointer to function to allocate memory block from the operating system.
     */
    osal_sysmem_alloc_func *sysmem_alloc_func;

    /** Pointer to function to release a block of memory back to operating system.
     */
    osal_sysmem_free_func *sysmem_free_func;

    /** Memory manager state structure, see osal_memory.h.
     */
    osalMemManagerState memstate;
#endif

	/* Resource monitor related, see osal_resource_monitor.c.
	 */
#if OSAL_RESOURCE_MONITOR

    /** Resource monitor state structure, see osal_resource_monitor.h.
     */
	osalResourceMonitorState resstate;
#endif

#if OSAL_CONSOLE
	/** Console state structure.
	 */
	osalConsoleState constate;
#endif

	/** System timer parameter. Values differ for each operating system.
	 */
	os_int64 sys_timer_param;

#if OSAL_FUNCTION_POINTER_SUPPORT
    /** Shut down function to close sockets library. Set if sockets library
        is initialized.
     */
    osal_shutdown_func *sockets_shutdown_func;
#endif
}
osalGlobalStruct;

#define osal_go() (!osal_global->exit_process)
#define osal_stop() (osal_global->exit_process)
