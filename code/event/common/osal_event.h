/**

  @file    event/common/osal_event.h
  @brief   Events.
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
#pragma once
#ifndef OSAL_EVENT_H_
#define OSAL_EVENT_H_
#include "eosal.h"

struct osalEventList;

/**
****************************************************************************************************
  Defines
****************************************************************************************************
*/

/** The event header structure allows chaining events to be set at exit.
 */
typedef struct osalEventHeader {
    struct osalEventHeader *next, *prev;
    struct osalEventList *list;
}
osalEventHeader;

/** Event Pointer Type: Event pointer returned by osal_event_create() function. Operating
    system specific functions cast their own event structure pointers to osalEvent pointers.
    If OSAL_OS_EVENT_LIST_SUPPORT is set, operating specific event structure starts with
    osalEventHeader structure.
 */
typedef struct osalEventHeader *osalEvent;


/** Head of linked event list, used for chaining events to be set at exit.
 */
typedef struct osalEventList {
    struct osalEventHeader *first, *last;
}
osalEventList;


/** Flags for osal_event_create function: If OSAL_EVENT_SET_AT_EXIT bit is set, the
    osal_event_add_to_atexit() is called to add the event to global list of events
    to set when exit process is requested. Use OSAL_EVENT_DEFAULT to indicate standard opeation.
 */
#define OSAL_EVENT_DEFAULT 0
#define OSAL_EVENT_SET_AT_EXIT 1

/** Wait infinitely. The OSAL_EVENT_INFINITE (-1) given as timeout_ms argument to
    osal_event_wait() will cause the function to block infinitely until the
    event is signaled.
 */
#define OSAL_EVENT_INFINITE -1
#define OSAL_EVENT_NO_WAIT 0


#if OSAL_MULTITHREAD_SUPPORT
/**
****************************************************************************************************

  @name Event functions

  The osal_event_create() function creates a new event, and osal_event_delete() deletes it.
  A thread may wait until an event is signaled or clear an event by osal_event_wait() function.
  Function osal_event_set() signals an event thus causing causes waiting thread to continue.

****************************************************************************************************
 */

/* Create a new event.
 */
osalEvent osal_event_create(
    os_short eflags);

/* Delete an event.
 */
void osal_event_delete(
    osalEvent evnt);

/* Set an event.
 */
void osal_event_set(
    osalEvent evnt);

/* Wait for an event.
 */
osalStatus osal_event_wait(
    osalEvent evnt,
    os_int timeout_ms);

/* Get pipe fd. (linux only)
 */
int osal_event_pipefd(
    osalEvent evnt);

/* Clear data buffered in pipe. (linux only)
 */
void osal_event_clearpipe(
    osalEvent evnt);

#if OSAL_OS_EVENT_LIST_SUPPORT
    /* Add event to list of events.
     */
    void osal_event_add_to_list(
        osalEventList *list,
        osalEvent evnt);

    /* Remove event from list of events.
     */
    void osal_event_remove_from_list(
        osalEvent evnt);

    /* Set all events in at exit list.
     */
    void osal_event_set_listed(
        osalEventList *list);

#else
    #define osal_event_add_to_list(l,e)
    #define osal_event_remove_from_list(l,e)
    #define osal_event_set_listed(l)
#endif

#else

/**
****************************************************************************************************

  @name Empty macros

  If OSAL_MULTITHREAD_SUPPORT flag is zero, these macros will replace functions and not generate
  any code. This allows to compile code which has calls mutex functions without multithreading
  support.

****************************************************************************************************
*/

  #define osal_event_create() OS_NULL
  #define osal_event_delete(x)
  #define osal_event_set(x)
  #define osal_event_wait(x,z) OSAL_SUCCESS

#endif
#endif
