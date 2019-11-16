/**

  @file    time/linux/osal_time.c
  @brief   Get and set system time.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Get or set system time (GMT) as long integer.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_TIME_SUPPORT

#include <time.h>


/**
****************************************************************************************************

  @brief Get system time (GMT).
  @anchor os_time

  The os_time() function get time from computer's clock. The time is always 64 bit integer,
  GMT in microseconds since epoc 1.1.1970.

  @param   t Pointer to 64 bit integer into which to store the time.
  @return  None.

****************************************************************************************************
*/
void os_time(
    os_int64 *t)
{
    struct timespec ts;

#ifdef CLOCK_REALTIME_COARSE
    if (!clock_gettime(CLOCK_REALTIME_COARSE, &ts))
    {
        goto goon;
    }
#endif
    if (clock_gettime(CLOCK_REALTIME, &ts))
    {
        osal_debug_error("os_time: Get system time failed");
        *t = 0;
        return;
    }
goon:
    *t = 1000000 * (os_long)ts.tv_sec + (os_long)ts.tv_nsec / 10000;
}


/**
****************************************************************************************************

  @brief Set system time (GMT).
  @anchor os_settime

  The os_settime() function sets computer's clock. 

  @param   t Time to set. 64 bit integer, microseconds since epoc 1.1.1970.
  @return  If the system time is succesfully set OSAL_SUCCESS (0). Other return values 
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus os_settime(
    const os_int64 *t)
{
    struct timespec ts;

    ts.tv_sec = (time_t)(*t / 1000000);
    ts.tv_nsec = (long)(*t % 1000000) * 1000;

    if (clock_settime(CLOCK_REALTIME, &ts))
    {
        return OSAL_STATUS_FAILED;
    }

	return OSAL_SUCCESS;
}

#endif
