/**

  @file    timer/linux/osal_timer.c
  @brief   System timer functions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  The os_get_timer() function gets the system timer as 64 bit integer, this is typically number
  of microseconds since the computer booted up. The os_elapsed() function checks if the
  specified time has elapsed.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#include <time.h>

/**
****************************************************************************************************

  @brief Initialize OSAL timers.
  @anchor osal_timer_initialize

  The osal_timer_initialize() function initializes OSAL timer module. This function is called by
  osal_initialize() and should not normally be called by application.

  @return  None.

****************************************************************************************************
*/
void osal_timer_initialize(
    void)
{
}


/**
****************************************************************************************************

  @brief Get system timer.
  @anchor os_get_timer

  The os_get_timer() function get current system timer value. System timer counts microseconds,
  typically since computer was booted.

  @param   start_t Pointer to 64 bit integer into which to store current system timer value, us.
  @return  None.

****************************************************************************************************
*/
void os_get_timer(
    os_timer *t)
{
    struct timespec ts;

#ifdef CLOCK_MONOTONIC_COARSE
    if (clock_gettime(CLOCK_MONOTONIC_COARSE, &ts))
    {
        if (clock_gettime(CLOCK_MONOTONIC, &ts))
        {
            osal_debug_error("os_get_timer: Get system timer failed");
            *t = 0;
            return;
        }
    }
#else
    if (clock_gettime(CLOCK_MONOTONIC, &ts))
    {
        osal_debug_error("os_get_timer: Get system timer failed");
        *t = 0;
        return;
    }
#endif
    *t = 1000000 * (os_long)ts.tv_sec + (os_long)ts.tv_nsec / 1000;
}

	
/**
****************************************************************************************************

  @brief Check if specific time period has elapsed, gets current timer value by os_get_timer().
  @anchor os_elapsed

  The os_elapsed() function checks if time period given as argument has elapsed since
  start time was recorded by os_get_timer() function.

  @param   start_t Pointer to 64 bit integer which contains start timer value as returned by 
           the os_get_timer() function.
  @param   period_ms Period length in milliseconds.
  @return  OS_TRUE (1) if specified time period has elapsed, or OS_FALSE (0) if not.

****************************************************************************************************
*/
os_boolean os_elapsed(
    os_timer *start_t,
    os_int period_ms)
{
    os_timer now_t, end_t;

	/* Calculate period end timer value in microseconds.
	 */
	osal_int64_set_long(&end_t, period_ms);
	osal_int64_multiply(&end_t, &osal_int64_1000);
	osal_int64_add(&end_t, start_t);

	/* Get current system timer value.
	 */
    os_get_timer(&now_t);

	/* If current timer value is past period end, then return OS_TRUE, or
	   OS_FALSE otherwise.
	 */
	return (os_boolean)(osal_int64_compare(&now_t, &end_t) >= 0);
}


/**
****************************************************************************************************

  @brief Check if specific time period has elapsed, current timer value given as argument.
  @anchor os_elapsed2

  The os_elapsed2() function checks if time period given as argument has elapsed since
  start time was recorded by os_get_timer() function.

  @param   start_t Pointer to 64 bit integer which contains start timer value as returned by 
           the os_get_timer() function.
  @param   now_t Pointer to 64 bit integer holding current system timer value.
  @param   period_ms Period length in milliseconds.
  @return  OS_TRUE (1) if specified time period has elapsed, or OS_FALSE (0) if not.

****************************************************************************************************
*/
os_boolean os_elapsed2(
    os_timer *start_t,
    os_timer *now_t,
    os_int period_ms)
{
	os_int64 end_t;

	/* Calculate period end timer value in microseconds.
	 */
	osal_int64_set_long(&end_t, period_ms);
	osal_int64_multiply(&end_t, &osal_int64_1000);
	osal_int64_add(&end_t, start_t);

	/* If current timer value is past period end, then return OS_TRUE, or
	   OS_FALSE otherwise.
	 */
	return (os_boolean)(osal_int64_compare(now_t, &end_t) >= 0);
}
