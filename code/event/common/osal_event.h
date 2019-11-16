/**

  @file    event/common/osal_event.h
  @brief   Events.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This header file contains functions prototypes and definitions for creating, deleting,
  setting and waiting events. An event is used to suspend a thread to wait for signal from
  an another thread of the same process.
  The osal_event_create() function creates a new event, and osal_event_delete() deletes it.
  A thread may wait until an event is signaled or clear an event by osal_event_wait() function.
  Function osal_event_set() signals an event thus causing causes waiting thread to continue.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_EVENT_INCLUDED
#define OSAL_EVENT_INCLUDED


/**
****************************************************************************************************

  @name Event Pointer Type.

  The osalEvent type is pointer to event. It is defined as pointer to dummy
  structure solely to provide compiler type checking. This sturcture is never really allocated,
  and OSAL functions cast their own event pointers to osalEvent pointers.


****************************************************************************************************
*/
/*@{*/

/* Dummy structure to provide compiler type checking.
 */
struct osalEventDummy;

/** Event pointer returned by osal_event_create() function.
 */
typedef struct osalEventDummy *osalEvent;

/*@}*/


/**
****************************************************************************************************

  @name Infinite Timeout

  Define OSAL_EVENT_INFINITE (-1) given as argument to osal_event_wait() function will disable
  time out.

****************************************************************************************************
*/
/*@{*/

/** Wait infinitely. The OSAL_EVENT_INFINITE (-1) given as timeout_ms argument to
    osal_event_wait() function will cause the function to block infinitely until the
    event is signaled.
 */
#define OSAL_EVENT_INFINITE -1
#define OSAL_EVENT_NO_WAIT 0

/*@}*/


#if OSAL_MULTITHREAD_SUPPORT 
/** 
****************************************************************************************************

  @name Event Functions

  The osal_event_create() function creates a new event, and osal_event_delete() deletes it.
  A thread may wait until an event is signaled or clear an event by osal_event_wait() function.
  Function osal_event_set() signals an event thus causing causes waiting thread to continue.

****************************************************************************************************
 */
/*@{*/

/* Create a new event.
 */
osalEvent osal_event_create(
    void);

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

/*@}*/

#else

/**
****************************************************************************************************

  @name Empty Event Macros

  If OSAL_MULTITHREAD_SUPPORT flag is zero, these macros will replace functions and not generate
  any code. This allows to compile code which has calls mutex functions without multithreading
  support.

****************************************************************************************************
*/
/*@{*/

  #define osal_event_create() OS_NULL
  #define osal_event_delete(x)
  #define osal_event_set(x)
  #define osal_event_wait(x,z) OSAL_SUCCESS

/*@}*/

#endif


#endif
