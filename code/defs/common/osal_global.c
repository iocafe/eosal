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


#if OSAL_PROCESS_CLEANUP_SUPPORT
/**
****************************************************************************************************

  @brief Request this process to exit.
  @anchor osal_request_exit

  The osal_request_exit() function sets global exit_process flag and sets all thread events listed
  in atexit event list so that threads can start shutting themselves down. Main thred which does
  eosal, etc, clean up should wait until child thread count reaches zero before final
  clean up.

  @return  Pointer to thread handle if OSAL_THREAD_ATTACHED flags is given, or OS_NULL otherwise.

****************************************************************************************************
*/
void osal_request_exit(void)
{
    if (!osal_global->exit_process) {
        osal_global->exit_process = OS_TRUE;
        osal_event_set_listed(&osal_global->atexit_events_list);
    }
}
#endif

