/**

  @file    timer/metal/osal_metal_timer.c
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
#define OSAL_INCLUDE_METAL_HEADERS
#include "eosal.h"
#ifdef OSAL_METAL


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
    *t = (os_timer)HAL_GetTick();
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
    os_uint diff;

    if (period_ms < 0) return OS_TRUE;
    diff = (os_uint)HAL_GetTick() - (os_uint)*start_t;
    return (os_boolean)((os_int)diff > period_ms);
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
    os_uint diff;

    if (period_ms < 0) return OS_TRUE;
    diff = (os_uint)*now_t - (os_uint)*start_t;
    /* Important, do signed compare: without this iocom, etc. may fail */
    return (os_boolean)((os_int)diff > period_ms);
}

#endif
