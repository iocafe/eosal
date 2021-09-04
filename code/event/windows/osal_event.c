/**

  @file    event/windows/osal_event.c
  @brief   Creating, deleting, setting and waiting for events.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  This file implements event related functionality for Windows. Generally events are used for
  a thread wait until it needs to do something.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#ifdef OSAL_WINDOWS
#if OSAL_MULTITHREAD_SUPPORT

/* Windows specific event structure.
 */
typedef struct osalWindowsEvent
{
#if OSAL_OS_EVENT_LIST_SUPPORT
    osalEventHeader hdr;
#endif
    HANDLE handle;
}
osalWindowsEvent;

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
    osalWindowsEvent *evnt;

    /* Allocate event handle structure and mark it initially not signaled. Pipes are not
     * created by default.
     */
    evnt = (osalWindowsEvent*)osal_sysmem_alloc(sizeof(osalWindowsEvent), OS_NULL);
    if (evnt == OS_NULL) {
        return OS_NULL;
    }
    os_memclear(evnt, sizeof(osalWindowsEvent));

    /* Call Windows to create an event.
     */
    evnt->handle = CreateEvent(NULL, FALSE, FALSE, NULL);

    /* If failed.
     */
    if (evnt->handle)
    {
        osal_debug_error("CreateEvent failed");
        osal_sysmem_free(evnt, sizeof(osalWindowsEvent));
        return OS_NULL;
    }

#if OSAL_OS_EVENT_LIST_SUPPORT
    if (eflags & OSAL_EVENT_SET_AT_EXIT) {
        osal_event_add_to_list(&osal_global->atexit_events_list, (osalEvent)pe);
    }
#endif

    /* Inform resource monitor that new event has been succesfullly created.
     */
    osal_resource_monitor_increment(OSAL_RMON_EVENT_COUNT);

    /* Return the event pointer.
     */
    return (osalEvent)evnt;
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
    osalWindowsEvent *e;

    if (evnt == NULL)
    {
        osal_debug_error("NULL event pointer");
        return;
    }
#if OSAL_OS_EVENT_LIST_SUPPORT
    osal_event_remove_from_list(&osal_global->atexit_events_list, evnt);
#endif

    /* Call Windows to delete the event.
     */
    if (CloseHandle(((osalWindowsEvent*)evnt)->handle))
    {
        /* Inform resource monitor that event has been deleted.
         */
        osal_resource_monitor_decrement(OSAL_RMON_EVENT_COUNT);
    }
    else {
        osal_debug_error("osal_event: CloseHandle failed");
        /* continue despite of error to free memory. */
    }

    osal_sysmem_free(evnt, sizeof(osalFreeRtosEvent));
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
    osal_debug_assert(evnt != OS_NULL);

#if OSAL_DEBUG
    if (evnt == NULL)
    {
        osal_debug_error("NULL event pointer");
        return;
    }
#endif

    if (!SetEvent(((osalWindowsEvent*)evnt)->handle))
    {
        osal_debug_error("SetEvent failed");
        return;
    }
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
           function will return OSAL_STATUS_TIMEOUT. To wait infinitely give
           OSAL_EVENT_INFINITE (-1) here. To check event state or to reset event to non
           signaled state without waiting set timeout_ms to 0.

  @return  If the event was signaled, either before the osal_event_wait call or during
           wait interval, the function will return OSAL_SUCCESS (0). If the function timed
           out and the event remained unsignaled, it will return OSAL_STATUS_TIMEOUT.
           Other values indicate failure, typically OSAL_STATUS_EVENT_FAILED.

****************************************************************************************************
*/
osalStatus osal_event_wait(
    osalEvent evnt,
    os_int timeout_ms)
{
    osal_debug_assert(evnt != OS_NULL);

    /* Make sure that infinite timeout matshes. This is propably unnecessary, check?
     */
    if (timeout_ms == OSAL_EVENT_INFINITE)
    {
        timeout_ms = INFINITE;
    }

    /* Call Windows wait function
     */
    switch (WaitForSingleObject((((osalWindowsEvent*)evnt)->handle), (DWORD)timeout_ms))
    {
        /* The state of the specified object is signaled.
         */
        case WAIT_OBJECT_0:
            return OSAL_SUCCESS;

        /* The time-out interval elapsed, and the object's state is nonsignaled.
         */
        case WAIT_TIMEOUT:
            return OSAL_STATUS_TIMEOUT;

        /* Some other problem.
         */
        default:
            osal_debug_error("WaitForSingleObject failed");
            return OSAL_STATUS_EVENT_FAILED;
    }
}

#endif
#endif
