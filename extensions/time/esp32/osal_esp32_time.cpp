/**

  @file    time/linux/osal_time.c
  @brief   Get and set system time.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Get or set system time (GMT) as long integer.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#ifdef OSAL_ESP32
#if OSAL_TIME_SUPPORT

#include "TimeLib.h"

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
    time_t tsec = now();
    *t = 1000000 * tsec;
}


/**
****************************************************************************************************

  @brief Set system time (GMT).
  @anchor os_settime

  The os_settime() function sets computer's clock. 

  @param   t Time to set. 64 bit integer, microseconds since epoc 1.1.1970.
  @return  If the system time is succesfully set OSAL_SUCCESS (0). Other return values 
           indicate an error. 

****************************************************************************************************
*/
osalStatus os_settime(
    const os_int64 *t)
{
    time_t tsec = (time_t)(*t / 1000000);
    setTime(tsec);
	return OSAL_SUCCESS;
}

#endif
#endif
