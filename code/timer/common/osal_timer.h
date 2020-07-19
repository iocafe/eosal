/**

  @file    timer/windows/osal_timer.h
  @brief   System timer functions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  This header file contains functions prototypes for reading system timer and checking if 
  specified time interval has elapsed.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

/** 
****************************************************************************************************

  @name System Timer Functions

  The os_get_timer() function gets the system timer as 64 bit integer, this is typically number
  of microseconds since the computer booted up. The os_has_elapsed() and os_has_elapsed_since()
  functions check if the specified time interval has elapsed.

****************************************************************************************************
 */
/*@{*/

/* Initialize OSAL timers.
 */
void osal_timer_initialize(
    void);

/* Get system timer, microseconds, typically time elapsed since last boot.
 */
void os_get_timer(
    os_timer *start_t);

/* Check if specific time period (milliseconds) has elapsed, gets current timer value by os_get_timer().
 */
os_boolean os_has_elapsed(
    os_timer *start_t,
    os_int period_ms);

/* Check if specific time period (milliseconds) has elapsed, current timer value given as argument.
 */
os_boolean os_has_elapsed_since(
    os_timer *start_t,
    os_timer *now_t,
    os_int period_ms);

/* Get number of milliseconds elapsed from start_t until now_t.
 */
os_long os_get_ms_elapsed(
    os_timer *start_t,
    os_timer *now_t);

/* If it time for a periodic event?
 */
os_boolean os_timer_hit(
    os_timer *memorized_t,
    os_timer *now_t,
    os_int period_ms);


/*@}*/
