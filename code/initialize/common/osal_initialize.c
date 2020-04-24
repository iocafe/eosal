/**

  @file    initialize/common/osal_initialize.c
  @brief   OSAL initialization and shut down.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  OSAL initialization and shut down.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"


/** Static allocation for global structure. This static allocation must never be referred
    directly, but through osal_global pointer. Reason is dynamic link library support.
    DLLs must be able to use the same global structure as the executable which loads
    them.
 */
static osalGlobalStruct osal_global_static;

/** Pointer to global OSAL state structure. The structure holds global state and settings
    of OSAL.
 */
osalGlobalStruct *osal_global = &osal_global_static;



/**
****************************************************************************************************

  @brief Initialize OSAL library for use.
  @anchor osal_initialize

  The osal_initialize() function initializes OSAL library for use. This function should
  be the first OSAL function called. The osal_shutdown() function cleans up resources
  used by the OSAL library.

  @param  flags Bit fields. OSAL_INIT_DEFAULT (0) for normal initalization.
          OSAL_INIT_NO_LINUX_SIGNAL_INIT not to initialize linux signals.

  @return  None.

****************************************************************************************************
*/
void osal_initialize(
    os_int flags)
{
    /* If OSAL library is already initialized, then do nothing.
     */
    if (osal_global->osal_initialized) return;

    /* Clear all globals. This is important if OSAL is shut down and then restarted.
     */
    os_memclear(osal_global, sizeof(osalGlobalStruct));

    /* Operating system specific initialization.
     */
    osal_init_os_specific(flags);

    /* Initialize memory management.
     */
#if OSAL_MEMORY_MANAGER
    osal_memory_initialize();
#endif

#if OSAL_DYNAMIC_MEMORY_ALLOCATION==0
    osal_static_mem_block_list = OS_NULL;
#endif

    /* Initialize mutexes. Creates system mutex.
     */
#if OSAL_MULTITHREAD_SUPPORT
    osal_mutex_initialize();
#endif

    /* Initialize timers.
     */
    osal_timer_initialize();

    /* Initialize system console.
     */
#if OSAL_CONSOLE
    osal_console_initialize();
#endif

    /* Mark that OSAL library is initialized
     */
    osal_global->osal_initialized = OS_TRUE;
}


/**
****************************************************************************************************

  @brief Shut down OSAL library.
  @anchor osal_shutdown

  The osal_shutdown() function cleans up resources used by the OSAL library.

  @return  None.

****************************************************************************************************
*/
void osal_shutdown(
    void)
{
    /* If OSAL library is not initialized, then do nothing.
     */
    if (!osal_global->osal_initialized) return;

#if OSAL_PROCESS_CLEANUP_SUPPORT

#if OSAL_FUNCTION_POINTER_SUPPORT
    /* Shutdown sockets library if it has been used, this is mostly for windows.
     */
    if (osal_global->sockets_shutdown_func)
    {
        osal_global->sockets_shutdown_func();
    }
#endif

    /* Shut down operating system specific functionality.
     */
    osal_shutdown_os_specific();

    /* Shut down mutexes. Releases system mutex.
     */
#if OSAL_MULTITHREAD_SUPPORT
    osal_mutex_shutdown();
#endif

    /* Shut down memory management.
     */
#if OSAL_MEMORY_MANAGER
    osal_memory_shutdown();
#endif

#endif /* OSAL_PROCESS_CLEANUP_SUPPORT */

    /* Mark that OSAL library is not initialized
     */
    osal_global->osal_initialized = OS_FALSE;
}
