/**

  @file    thread/esp32/osal_esp32_sleep.cpp
  @brief   Thread sleep/delay.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    31.5.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#ifdef OSAL_ESP32

#if OSAL_MULTITHREAD_SUPPORT
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif 


/**
****************************************************************************************************

  @brief Suspend thread execution for a specific time, milliseconds.
  @anchor os_sleep

  The os_sleep() function suspends the execution of the current thread for a specified
  interval. The function is used for both to create timed delays and to force scheduler to give
  processor time to lower priority threads. If time_ms is zero the function suspends execution
  of the thread until end of current processor time slice.

  @param   time_ms Time to sleep, milliseconds. Value 0 sleeps until end of current processor
           time slice.
  @return  None.

****************************************************************************************************
*/
void os_sleep(
    os_long time_ms)
{
#if OSAL_MULTITHREAD_SUPPORT
    time_ms /= portTICK_PERIOD_MS;
    if (time_ms < 1) time_ms = 1;
    vTaskDelay((TickType_t)time_ms);
#else
    delay(time_ms);
#endif
}


/**
****************************************************************************************************

  @brief Suspend thread execution for a specific time, microseconds.
  @anchor os_microsleep

  The os_microsleep() function suspends the execution of the current thread for a specified
  interval. The function is used for both to create timed delays and to force scheduler to give
  processor time to lower priority threads. If time_ms is zero the function suspends execution
  of the thread until end of current processor time slice.

  Arduino specific: The function support only one millisecond precision.

  @param   time_us Time to sleep, microseconds. Value 0 sleeps until end of current processor
           time slice.
  @return  None.

****************************************************************************************************
*/
void os_microsleep(
    os_long time_us)
{
#if OSAL_MULTITHREAD_SUPPORT
    os_sleep(time_us / (portTICK_PERIOD_MS * 1000));
#else
    delay(time_us / 1000);
#endif
}

#endif
