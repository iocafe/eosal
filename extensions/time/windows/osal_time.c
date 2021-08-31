/**

  @file    time/windows/osal_time.c
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
#ifdef OSAL_WINDOWS
#if OSAL_TIME_SUPPORT


/* 64 bit integer constants for clock.
 */
#if OSAL_LONG_IS_64_BITS

/** 64 bit constant global variable holding value 10000. Do not modify the value.
 */
static const os_int64 osal_clock_10 = 10;

/** Difference between Windows file time zero and 1.1.1970 epoch.
 */
static const os_int64 osal_clock_win_file_time_offset = 0x295E9648864000 /* Was 0xA9730B66800 */;

#else

/** 64 bit constant global variable holding value 10000. Do not modify the value.
 */
static const os_int64 osal_clock_10 = {{10, 0, 0, 0}};

/** Difference between Windows file time zero and 1.1.1970 epoch.
 */
static const os_int64 osal_clock_win_file_time_offset = {{0x4000, 0x4886, 0x5E96, 0x29}}; 

#endif


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
	FILETIME wintime;
	os_int64 osaltime;

	/* The GetSystemTime function retrieves the current system date and time as Coordinated 
	   Universal Time (UTC).
	 */
	GetSystemTimeAsFileTime(&wintime);

	/* Convert to OSAL time, GMT, microseconds since 1.1.1970.
	 */
	osal_int64_set_uint2(&osaltime, wintime.dwLowDateTime, wintime.dwHighDateTime);
	osal_int64_divide(&osaltime, &osal_clock_10);
	osal_int64_subtract(&osaltime, &osal_clock_win_file_time_offset);
	osal_int64_copy(t, &osaltime);
}


/**
****************************************************************************************************

  @brief Set system time (GMT).
  @anchor os_settime

  The os_settime() function sets computer's clock. The time is always 64 bit integer,
  microseconds since epoc 1.1.1970.

  @param   t Time to set.

  @return  If the system time is succesfully set OSAL_SUCCESS (0). Other return values 
           indicate an error. 

****************************************************************************************************
*/
osalStatus os_settime(
    const os_int64 *t)
{
	FILETIME wintime;
	SYSTEMTIME winsystime;
	os_int64 osaltime;

	/* Convert OSAL time to Windows filetime.
	 */
	osal_int64_copy(&osaltime, t);
	osal_int64_add(&osaltime, &osal_clock_win_file_time_offset);
	osal_int64_multiply(&osaltime, &osal_clock_10);
	osal_int64_get_uint2(&osaltime, &wintime.dwLowDateTime, &wintime.dwHighDateTime);

	/* The FileTimeToSystemTime function converts a 64-bit file time to system time format. 
	 */
	if (!FileTimeToSystemTime(&wintime, &winsystime))
	{
		osal_debug_error("Time conversion failed");
		return OSAL_STATUS_CLOCK_SET_FAILED;
	}

	/* The SetSystemTime function sets the current system time and date. The system time 
	   is expressed in Coordinated Universal Time (UTC).
	 */
	if (!SetSystemTime(&winsystime))
	{
		return OSAL_STATUS_CLOCK_SET_FAILED;
	}

	return OSAL_SUCCESS;
}

#endif
#endif
