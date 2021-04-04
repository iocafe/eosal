/**

  @file    event/common/osal_os_event_list.c
  @brief   Maintaining list of events to set when process exits.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  This header file contains functions prototypes and definitions for creating, deleting,
  setting and waiting events. An event is used to suspend a thread to wait for signal from
  an another thread of the same process.
  The osal_event_create() function creates a new event, and osal_event_delete() deletes it.
  A thread may wait until an event is signaled or clear an event by osal_event_wait() function.
  Function osal_event_set() signals an event thus causing causes waiting thread to continue.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#if OSAL_MULTITHREAD_SUPPORT
#if OSAL_OS_EVENT_LIST_SUPPORT

/* Add event to list of events to set at exit.
 */
void osal_event_add_to_list(
    osalEventList *list,
    osalEvent evnt)
{
    osal_debug_assert(osal_global->system_mutex != OS_NULL);

    if (evnt->next) {
        osal_event_remove_from_list(list, evnt);
    }

    os_lock();
    if (list->last) {
        list->last->next = evnt;
        evnt->prev = list->last;
        list->last = evnt;
    }
    else {
        list->last = list->first = evnt;
    }
    os_unlock();
}


/* Remove event from list of events to set at exit.
 */
void osal_event_remove_from_list(
    osalEventList *list,
    osalEvent evnt)
{
    if (evnt->next == OS_NULL) return;

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
    os_unlock();
}

/* Set all events in at exit list.
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
