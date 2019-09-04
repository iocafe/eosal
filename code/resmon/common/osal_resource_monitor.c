/**

  @file    resmon/common/osal_resource_monitor.c
  @brief   Monitor operating system resource use.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  It is important to ensure that software doesn't have cumulative hidden programming errors
  which do manifest only after time. Often these are caused by allocating resources which are
  not released. Thus OSAL needs to keep track of allocated memory, handle counts, thread counts,
  mutex counts, event counts...
  Once software is tested and ready for final release this tracking code can be turned off
  by setting OSAL_RESOURCE_MONITOR flag to zero.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
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

  @param   ix Resource counter index. Selects which resource counter to be updated.
  @param   delta Change, positive or negative. Selects how much the counter is to be changed.

  @return  None.

****************************************************************************************************
*/
void osal_resource_monitor_update(
    osalResourceIndex ix,
    os_memsz delta)
{
	os_memsz 
		v;

	/* Update current resource counter value
	 */
	osal_global->resstate.current[ix] += (os_memsz)delta;

	/** Record the peak value.
	 */
	v = osal_global->resstate.current[ix];
	if (v > osal_global->resstate.peak[ix]) 
	{
		osal_global->resstate.peak[ix] = v;
	}

	/* Flag resource changed.
	 */
	osal_global->resstate.changed[ix] = OS_TRUE;
	osal_global->resstate.updated = OS_TRUE;
}


/**
****************************************************************************************************

  @brief Get resource counter value.
  @anchor osal_resource_monitor_get_value

  The osal_resource_monitor_get_value() function gets resource counter value, either current or 
  peak value.

  @param   ix Resource counter index. Selects the resource counter.
  @param   flags Either OSAL_RMON_CURRENT to get current value of the resource counter, or
		   OSAL_RMON_PEAK to get resource counter's recorded maximum value.

  @return  Resource counter value.

****************************************************************************************************
*/
os_long osal_resource_monitor_get_value(
    osalResourceIndex ix,
	os_int flags)
{
	/* If resource index is illegal, return -1.
	 */
	if (ix < 1 || ix >= OSAL_RMON_COUNTERS_SZ) return -1;

	/* Return either current resource counter value or the peak value.
	 */
	return (flags & OSAL_RMON_PEAK) 
		? osal_global->resstate.peak[ix]
		: osal_global->resstate.current[ix];
}


/**
****************************************************************************************************

  @brief Check for changed resource counter.
  @anchor osal_resource_monitor_get_change

  The osal_resource_monitor_get_change() function checks for changed resource counters.

  @return  Index of changes resource counter, or OSAL_RMON_NONE (0) if no resource
		   counter has changed.

****************************************************************************************************
*/
osalResourceIndex osal_resource_monitor_get_change(
	void)
{
	osalResourceIndex
		ix;

	if (osal_global->resstate.updated)
	{
		osal_global->resstate.updated = OS_FALSE;

		for (ix = 1; ix < OSAL_RMON_COUNTERS_SZ; ++ix)
		{
			if (osal_global->resstate.changed[ix]) 
			{
				osal_global->resstate.changed[ix] = OS_FALSE;
				return ix;
			}
		}
	}

	return OSAL_RMON_NONE;
}

#endif
