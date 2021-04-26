/**

  @file    event/common/osal_os_event_list.c
  @brief   Maintaining list of events to set when process exits.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#if OSAL_MULTITHREAD_SUPPORT
#if OSAL_OS_EVENT_LIST_SUPPORT


/**
****************************************************************************************************

  @brief Add event to list of events.

  The osal_event_add_to_list() function deletes an event which was created by osal_event_create()
  function. Resource monitor's event count is decremented, if resource monitor is enabled.

  Linked lists of events are used to set multiple events using list, typically when
  the process exists.

  @param   list Root of linked list of OS events.
  @param   evnt Event to add to list.

****************************************************************************************************
*/
void osal_event_add_to_list(
    osalEventList *list,
    osalEvent evnt)
{
    osal_debug_assert(osal_global->system_mutex != OS_NULL);

    os_lock();

    if (evnt->list) {
        osal_event_remove_from_list(evnt);
    }

    if (list->last) {
        list->last->next = evnt;
        evnt->prev = list->last;
        list->last = evnt;
    }
    else {
        list->last = list->first = evnt;
    }
    evnt->list = list;
    os_unlock();
}


/**
****************************************************************************************************

  @brief Remove event from list of events to set at exit.

  The osal_event_remove_from_list() function removes the event from any linked list the event
  my be in. If event is not part of list, the function does nothing.

  @param   evnt Event to remove from list.

****************************************************************************************************
*/
void osal_event_remove_from_list(
    osalEvent evnt)
{
    osalEventList *list;

    list = evnt->list;
    if (list == OS_NULL) return;

    os_lock();
    if (evnt->next) {
        evnt->next->prev = evnt->prev;
    }
    else {
        list->last = evnt->prev;
    }
    if (evnt->prev) {
        evnt->prev->next = evnt->next;
    }
    else {
        list->first = evnt->next;
    }
    evnt->next = evnt->prev = OS_NULL;
    evnt->list = OS_NULL;
    os_unlock();
}


/**
****************************************************************************************************

  @brief Set all events in at exit list.

  The osal_event_set_listed() function sets all events on linked list.

  @param   list Root of linked list of OS events.

****************************************************************************************************
*/
void osal_event_set_listed(
    osalEventList *list)
{
    osalEvent evnt;

    if (list->first == OS_NULL) return;

    os_lock();
    for (evnt = list->first; evnt; evnt = evnt->next)
    {
        osal_event_set(evnt);
    }
    os_unlock();
}

#endif
#endif
