/**

  @file    timer/duino/osal_duino_timer.c
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
#ifdef OSAL_ARDUINO


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

  This function can be called from interrupt handler.

  @param   start_t Pointer to integer into which to store current system timer value.
  @return  None.

****************************************************************************************************
*/
void OS_ISR_FUNC_ATTR os_get_timer(
    os_timer *t)
{
    *t = (os_timer)millis();
}

	
/**
****************************************************************************************************

  @brief Check if specific time period has elapsed, gets current timer value by os_get_timer().
  @anchor os_has_elapsed

  The os_has_elapsed() function checks if time period given as argument has elapsed since
  start time was recorded by os_get_timer() function.

  This function can be called from interrupt handler.

  @param   start_t Start timer value as set to t by the os_get_timer() function.
  @param   period_ms Period length in milliseconds.
  @return  OS_TRUE (1) if specified time period has elapsed, or OS_FALSE (0) if not.

****************************************************************************************************
*/
os_boolean OS_ISR_FUNC_ATTR os_has_elapsed(
    os_timer *start_t,
    os_int period_ms)
{
    os_uint diff;

    if (period_ms < 0) return OS_TRUE;
    diff = (os_uint)millis() - (os_uint)*start_t;
    return (os_boolean)((os_int)diff > period_ms);
}


/**
****************************************************************************************************

  @brief Check if specific time period has elapsed, current timer value given as argument.
  @anchor os_has_elapsed_since

  The os_has_elapsed_since() function checks if time period given as argument has elapsed since
  start time was recorded by os_get_timer() function.

  This function can be called from interrupt handler.

  @param   start_t Start timer value as set to t by the os_get_timer() function.
  @param   now_t Current system timer value as set to t by the os_get_timer() function.
  @param   period_ms Period length in milliseconds.
  @return  OS_TRUE (1) if specified time period has elapsed, or OS_FALSE (0) if not.

****************************************************************************************************
*/
os_boolean OS_ISR_FUNC_ATTR os_has_elapsed_since(
    os_timer *start_t,
    os_timer *now_t,
    os_int period_ms)
{
    os_uint diff;

    if (period_ms < 0) return OS_TRUE;
    diff = (os_uint)*now_t - (os_uint)*start_t;

    /* Important, do signed compare: without this iocom, etc. may fail!
     */
    return (os_boolean)((os_int)diff > period_ms);
}


/**
****************************************************************************************************

  @brief Get number of milliseconds elapsed from start_t until now_t.
  @anchor os_os_get_ms_elapsed

  The os_os_get_ms_elapsed() function...

  This function can be called from interrupt handler.

  @param   start_t Start timer value as set to t by the os_get_timer() function.
  @param   now_t Current system timer value as set to t by the os_get_timer() function.
  @return  Number of milliseconds.

****************************************************************************************************
*/
os_long OS_ISR_FUNC_ATTR os_get_ms_elapsed(
    os_timer *start_t,
    os_timer *now_t)
{
    os_uint diff;

    diff = (os_uint)*now_t - (os_uint)*start_t;
    return (os_long)diff;
}


/**
****************************************************************************************************

  @brief If it time for a periodic event?
  @anchor os_has_elapsed_since

  The os_timer_hit() function returns OS_TRUE if it is time to do a periodic event. The function
  keeps events times to be divisible by period (from initialization of memorized_t). If this
  function is called that rarely that skew is one or more whole periods, events what happended
  on that time will be skipped.

  This function can be called from interrupt handler.

  @param   memorized_t Memorized timer value to keep track of events, can be zero initially.
  @param   now_t Current system timer value as set to t by the os_get_timer() function.
  @param   period_ms Period how often to get "hits" in milliseconds.
  @return  OS_TRUE (1) if "hit", or OS_FALSE (0) if not.

****************************************************************************************************
*/
os_boolean OS_ISR_FUNC_ATTR os_timer_hit(
    os_timer *memorized_t,
    os_timer *now_t,
    os_int period_ms)
{
    os_uint diff, u, m, n;

    if (period_ms <= 0) return OS_TRUE;
    u = (os_uint)*now_t;
    m = (os_uint)*memorized_t;
    diff = u - m;

    /* Important, do signed compare: without this iocom, etc. may fail.
     */
    if ((os_int)diff < period_ms) return OS_FALSE;

    /* If we have a skew more than a period, skip hit times.
     */
    if (diff >= (os_uint)2*period_ms)
    {
        n = diff / period_ms;
        m += n * period_ms;
    }
    else
    {
        m += (os_uint)period_ms;
    }

    *memorized_t = m;
    return OS_TRUE;
}

#endif
