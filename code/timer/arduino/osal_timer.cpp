/**

  @file    timer/arduino/osal_timer.c
  @brief   System timer functions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

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
// #include "esp_timer/include/esp_timer.h"
#include "esp_timer.h"

#include "Arduino.h"

#if 0
// Writing directly to  watch dog timers

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
void feedTheDog(){
  // feed dog 0
  TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE; // write enable
  TIMERG0.wdt_feed=1;                       // feed dog
  TIMERG0.wdt_wprotect=0;                   // write protect
  // feed dog 1
  TIMERG1.wdt_wprotect=TIMG_WDT_WKEY_VALUE; // write enable
  TIMERG1.wdt_feed=1;                       // feed dog
  TIMERG1.wdt_wprotect=0;                   // write protect
}
#endif

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

  @param   start_t Pointer to integer into which to store current system timer value.
  @return  None.

****************************************************************************************************
*/
void os_get_timer(
    os_timer *t)
{
#ifdef ESP_PLATFORM
    *t = (os_timer)esp_timer_get_time();
#else
    *t = (os_timer)millis();
#endif
}

	
/**
****************************************************************************************************

  @brief Check if specific time period has elapsed, gets current timer value by os_get_timer().
  @anchor os_has_elapsed

  The os_has_elapsed() function checks if time period given as argument has elapsed since
  start time was recorded by os_get_timer() function.

  @param   start_t Start timer value as set to t by the os_get_timer() function.
  @param   period_ms Period length in milliseconds.
  @return  OS_TRUE (1) if specified time period has elapsed, or OS_FALSE (0) if not.

****************************************************************************************************
*/
os_boolean os_has_elapsed(
    os_timer *start_t,
    os_int period_ms)
{
#ifdef ESP_PLATFORM
    return (os_boolean)(esp_timer_get_time() >= *start_t + 1000 * (os_timer)period_ms);
#else
    os_uint diff;

    if (period_ms < 0) return OS_TRUE;
    diff = (os_uint)millis() - (os_uint)*start_t;
    return (os_boolean)((os_int)diff > period_ms);
#endif
}


/**
****************************************************************************************************

  @brief Check if specific time period has elapsed, current timer value given as argument.
  @anchor os_has_elapsed_since

  The os_has_elapsed_since() function checks if time period given as argument has elapsed since
  start time was recorded by os_get_timer() function.

  @param   start_t Start timer value as set to t by the os_get_timer() function.
  @param   now_t Current system timer value as set to t by the os_get_timer() function.
  @param   period_ms Period length in milliseconds.
  @return  OS_TRUE (1) if specified time period has elapsed, or OS_FALSE (0) if not.

****************************************************************************************************
*/
os_boolean os_has_elapsed_since(
    os_timer *start_t,
    os_timer *now_t,
    os_int period_ms)
{
#ifdef ESP_PLATFORM
    return (os_boolean)(*now_t >= *start_t + 1000 * (os_timer)period_ms);
#else
    os_uint diff;

    if (period_ms < 0) return OS_TRUE;
    diff = (os_uint)*now_t - (os_uint)*start_t;

    /* Important, do signed compare: without this iocom, etc. may fail!
     */
    return (os_boolean)((os_int)diff > period_ms);
#endif
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
os_int os_get_ms_elapsed(
    os_timer *start_t,
    os_timer *now_t)
{
#ifdef ESP_PLATFORM
    return (os_int)(*now_t - *start_t) / 1000;
#else
    os_uint diff;

    diff = (os_uint)*now_t - (os_uint)*start_t;
    return (os_int)diff;
#endif
}


/**
****************************************************************************************************

  @brief If it time for a periodic event?
  @anchor os_has_elapsed_since

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
#ifdef ESP_PLATFORM
    os_int64 diff, n, m, period_us;

    if (period_ms <= 0) return OS_TRUE;

    /* If not enough time has elapsed.
     */
    m = *memorized_t;
    diff = *now_t - m;
    period_us = 1000 * (os_timer)period_ms;
    if (diff < (os_timer)period_us) return OS_FALSE;

    /* If we have a skew more than a period, skip hit times.
     */
    if (diff >= 2 * period_us)
    {
        n = diff / period_us;
        m += n * period_us;
    }
    else
    {
        m += period_us;
    }

    *memorized_t = m;
    return OS_TRUE;
#else
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
#endif
}

