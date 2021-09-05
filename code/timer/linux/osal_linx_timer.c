/**

  @file    timer/linux/osal_linux_timer.c
  @brief   System timer functions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  The os_get_timer() function gets the system timer as 64 bit integer, this is typically number
  of microseconds since the computer booted up. The os_has_elapsed() function checks if the
  specified time has elapsed.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#ifdef OSAL_LINUX
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
  @anchor os_has_elapsed

  The os_has_elapsed() function checks if time period given as argument has elapsed since
  start time was recorded by os_get_timer() function.

  @param   start_t Pointer to 64 bit integer which contains start timer value as returned by 
           the os_get_timer() function.
  @param   period_ms Period length in milliseconds.
  @return  OS_TRUE (1) if specified time period has elapsed, or OS_FALSE (0) if not.

****************************************************************************************************
*/
os_boolean os_has_elapsed(
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
  @anchor os_has_elapsed_since

  The os_has_elapsed_since() function checks if time period given as argument has elapsed since
  start time was recorded by os_get_timer() function.

  @param   start_t Pointer to 64 bit integer which contains start timer value as returned by 
           the os_get_timer() function.
  @param   now_t Pointer to 64 bit integer holding current system timer value.
  @param   period_ms Period length in milliseconds.
  @return  OS_TRUE (1) if specified time period has elapsed, or OS_FALSE (0) if not.

****************************************************************************************************
*/
os_boolean os_has_elapsed_since(
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


/**
****************************************************************************************************

  @brief Get number of milliseconds elapsed from start_t until now_t.
  @anchor os_os_get_ms_elapsed

  The os_os_get_ms_elapsed() function...

  @param   start_t Start timer value as set to t by the os_get_timer() function.
  @param   now_t Current system timer value as set to t by the os_get_timer() function.
  @return  Number of milliseconds.

****************************************************************************************************
*/
os_long os_get_ms_elapsed(
    os_timer *start_t,
    os_timer *now_t)
{
    os_int64 tv;

    /* Calculate period end timer value in microseconds.
     */
    osal_int64_copy(&tv, now_t);
    osal_int64_subtract(&tv, start_t);

    osal_int64_divide(&tv, &osal_int64_1000);
    return osal_int64_get_long(&tv);
}


/**
****************************************************************************************************

  @brief If it time for a periodic event?
  @anchor os_timer_hit

  The os_timer_hit() function returns OS_TRUE if it is time to do a periodic event. The function
  keeps events times to be divisible by period (from initialization of memorized_t). If this
  function is called that rarely that skew is one or more whole periods, events what happended
  on that time will be skipped.

  @param   memorized_t Memorized timer value to keep track of events, can be zero initially.
  @param   now_t Current system timer value as set to t by the os_get_timer() function.
  @param   period_ms Period how often to get "hits" in milliseconds.
  @return  OS_TRUE (1) if "hit", or OS_FALSE (0) if not.

****************************************************************************************************
*/
os_boolean os_timer_hit(
    os_timer *memorized_t,
    os_timer *now_t,
    os_int period_ms)
{
    os_int64 diff, p, p2;

    if (period_ms <= 0) return OS_TRUE;

    osal_int64_set_long(&p, period_ms);
    osal_int64_multiply(&p, &osal_int64_1000);
    osal_int64_copy(&diff, now_t);
    osal_int64_subtract(&diff, memorized_t);

    /* If not enough time has elapsed.
     */
    if (osal_int64_compare(&diff, &p) < 0) return OS_FALSE;

    /* If we have a skew more than a period, skip hit times.
     */
    osal_int64_copy(&p2, &p);
    osal_int64_add(&p2, &p);
    if (osal_int64_compare(&diff, &p2) >= 0)
    {
        osal_int64_divide(&diff, &p);
        osal_int64_multiply(&diff, &p);
        osal_int64_add(memorized_t, &diff);
    }
    else
    {
        osal_int64_add(memorized_t, &p);
    }

    return OS_TRUE;
}

#endif
