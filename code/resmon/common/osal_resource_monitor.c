/**

  @file    osal_resource_monitor.c
  @brief   Monitor operating system resource use.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    17.7.2020

  We monitor use of operating system resources to ensure that that we will not have cumulative
  programming errors (memory leaks, etc) and that we do not transfer unnecessary data over
  communication.

  This resorce tracking code can be excluded from build by defining OSAL_RESOURCE_MONITOR as 0.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"

#if OSAL_RESOURCE_MONITOR

/**
****************************************************************************************************

  @brief Update resource use.
  @anchor osal_resource_monitor_update

  The osal_resource_monitor_update() function updates a resource counter. The function modifies
  the current resource counter value, records the peak values and flags the resource counter
  as updated.

  This function must be light and NOT call os_lock(), forward resource changed, etc.
  It is called from low level code.

  @param   ix Resource counter index. Selects which resource counter to be updated.
  @param   delta Change, positive or negative. Selects how much the counter is to be changed.

  @return  None.

****************************************************************************************************
*/
void osal_resource_monitor_update(
    osalResourceIndex ix,
    os_memsz delta)
{
    /* Update current resource counter value
     */
    osal_global->resstate.current[ix] += delta;

    /* Flag resource changed.
     */
    osal_global->resstate.changed[ix] = OS_TRUE;
    osal_global->resstate.updated = OS_TRUE;
}

#endif
