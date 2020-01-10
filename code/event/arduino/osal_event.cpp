/**

  @file    event/arduino/osal_event.c
  @brief   Creating, deleting, setting and waiting for events.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  This file implements events using FreeRTOS binary semaphores. Generally events are used by
  a thread to wait until something happens.

  See https://www.freertos.org/xSemaphoreCreateBinary.html

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"

#if OSAL_MULTITHREAD_SUPPORT

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
****************************************************************************************************

  @brief Create an event.
  @anchor osal_event_create

  The osal_event_create() function creates an event. An event is used to suspend a thread to wait
  for signal from an another thread of the same process. The new event is initially in nonsignaled
  state, so immediate osal_event_wait() for new event will block execution.

  Resource monitor's event count is incremented, if resource monitor is enabled.

  @return  evnt Event pointer. If the function fails, it returns OS_NULL.

****************************************************************************************************
*/
osalEvent osal_event_create(
    void)
{
    SemaphoreHandle_t m;

    /* Use semaphora as event
     */
    m = xSemaphoreCreateBinary();
    if (m == NULL)
    {
        osal_debug_error("osal_event.c, xSemaphoreCreateBinary() failed");
        return OS_NULL;
    }

    /* Inform resource monitor that event has been created and return the event pointer.
     */
    osal_resource_monitor_increment(OSAL_RMON_EVENT_COUNT);
    return (osalEvent)m;
}


/**
****************************************************************************************************

  @brief Delete an event.
  @anchor osal_event_delete

  The osal_event_delete() function deletes an event which was created by osal_event_create()
  function. Resource monitor's event count is decremented, if resource monitor is enabled.

  @param   evnt Pointer to event to delete. 

  @return  None.

****************************************************************************************************
*/
void osal_event_delete(
    osalEvent evnt)
{
    if (evnt == OS_NULL)
    {
        osal_debug_error("osal_event_delete: NULL argument");
        return;
    }

    vSemaphoreDelete((SemaphoreHandle_t)evnt);

    /* Inform resource monitor that the event has been deleted.
     */
    osal_resource_monitor_decrement(OSAL_RMON_EVENT_COUNT);
}


/**
****************************************************************************************************

  @brief Set an event.
  @anchor osal_event_set

  The osal_event_set() function signals an event, which will allow waiting thread to proceed.
  The event object remains signaled until a single waiting thread is released, at which time
  the system sets the state to nonsignaled. If no threads are waiting, the event object's state
  remains signaled, and first following osal_event_wait() call will return immediately.

  @param   evnt Event pointer returned by osal_event_create() function.
  @return  None.

****************************************************************************************************
*/
void osal_event_set(
    osalEvent evnt)
{
    if (evnt == OS_NULL)
    {
        osal_debug_error("osal_event_set: NULL argument");
        return;
    }

    // xSemaphoreTake(evnt, 0);
    // xSemaphoreGive(evnt); For unknown reason crash here at random.

    BaseType_t xx;
    xSemaphoreGiveFromISR(evnt, &xx);

}


/**
****************************************************************************************************

  @brief Wait for an event.
  @anchor osal_event_wait

  The osal_event_wait() function causes this thread to wait for event. If the event's state is
  nonsignaled, the calling thread enters an efficient wait state. The is done by operating
  system and expected to consume very little processor time while waiting. If event state
  is signaled either before the function call or during wait interval function returns
  OSAL_SUCCESS. When the function returns the event is always cleared to non signaled state.

  @param   evnt Event pointer returned by osal_event_create() function.
  @param   timeout_ms Wait timeout. If event is not signaled within this time, then the
           function will return OSAL_STATUS_TIMEOUT. To wait infinetly give
           OSAL_EVENT_INFINITE (-1) here. To check event state or to reset event to non
           signaled state without waiting set timeout_ms to 0.

  @return  If the event was signaled, either before the osal_event_wait call or during
           wait interval, the function will return OSAL_SUCCESS (0). If the function timed
           out and the event remained unsignaled, it will return OSAL_STATUS_TIMEOUT.
           Other values indicate failure, typically OSAL_STATUS_EVENT_FAILED.
           See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_event_wait(
    osalEvent evnt,
    os_int timeout_ms)
{
    TickType_t tout_ticks;

    if (evnt == OS_NULL)
    {
        osal_debug_error("osal_event_set: NULL argument");
        return OSAL_STATUS_FAILED;
    }

#if INCLUDE_vTaskSuspend != 1
    #error BREAK COMPILE: We need INCLUDE_vTaskSuspend=1 FreeRTOS configuration define for portMAX_DELAY
#endif

    tout_ticks = (timeout_ms == OSAL_EVENT_INFINITE) ? portMAX_DELAY : timeout_ms/portTICK_PERIOD_MS;

    return ((xSemaphoreTake(evnt, tout_ticks) == pdTRUE)
            ? OSAL_SUCCESS : OSAL_STATUS_TIMEOUT);
}

#endif
