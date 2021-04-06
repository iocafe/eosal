/**

  @file    defs/common/osal_global.c
  @brief   Global OSAL state.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    24.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"

/* Some common strings.
 */
OS_CONST os_char osal_str_asterisk[] = "*";
OS_CONST os_char osal_str_empty[] = "";


#if OSAL_PROCESS_CLEANUP_SUPPORT && OSAL_MULTITHREAD_SUPPORT
/**
****************************************************************************************************

  @brief Request this process to exit.
  @anchor osal_request_exit

  The osal_request_exit() function sets global exit_process flag and sets all thread events listed
  in atexit event list so that threads can start shutting themselves down. Main thred which does
  eosal, etc, clean up should wait until child thread count reaches zero before final
  clean up.

  The osal_wait_for_threads_to_exit() function is used to check when all worker threads have
  terminated themselves.

  @return  Pointer to thread handle if OSAL_THREAD_ATTACHED flags is given, or OS_NULL otherwise.

****************************************************************************************************
*/
void osal_request_exit(void)
{
    osal_global->exit_process = OS_TRUE;
    osal_event_set_listed(&osal_global->atexit_events_list);
}


/**
****************************************************************************************************

  @brief Wait until all worker threads have exited.
  @anchor osal_wait_for_threads_to_exit

  To shut down a process in orderly way, the all worker threads are gracefully terminated
  on their own volition before process exists. To know when all thread, including detached
  ones are terminated we use volatile global variable thread_count, which tracks how many
  threads have been created.

  THe os_lock() MUST NOT be on when this function is called: Worker threads may need os_lock()
  to be able to terminate.

  @return  Pointer to thread handle if OSAL_THREAD_ATTACHED flags is given, or OS_NULL otherwise.

****************************************************************************************************
*/
void osal_wait_for_threads_to_exit(void)
{
    while (osal_global->thread_count > 0) {
        os_timeslice();
    }
}

#endif

