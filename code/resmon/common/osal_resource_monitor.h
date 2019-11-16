/**

  @file    include/osal_resource_monitor.h
  @brief   Monitor operating system resource use.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  It is important to ensure that software doesn't have cumulative hidden programming errors
  which do manifest only after time. Often these are caused by allocating resources which are
  not released. Thus OSAL needs to keep track of allocated memory, handle counts, thread counts,
  mutex counts, event counts...
  Once software is tested and ready for final release this tracking code can be turned off
  by setting OSAL_RESOURCE_MONITOR flag to zero.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_RESOURCE_MONITOR_INCLUDED
#define OSAL_RESOURCE_MONITOR_INCLUDED


/**
****************************************************************************************************

  @name Resource Monitor Macros

  Resource monitor macros are mapped to resource monitor functions when OSAL_RESOURCE_MONITOR
  flag is nonzero. If OSAL_RESOURCE_MONITOR flag is zero, these macros will not generate any code.

****************************************************************************************************
*/
/*@{*/

#if OSAL_RESOURCE_MONITOR
  /** Log a programming error.
   */
  #define osal_resource_monitor_increment(ix) osal_resource_monitor_update(ix, 1)
  #define osal_resource_monitor_decrement(ix) osal_resource_monitor_update(ix, -1)
#else
  #define osal_resource_monitor_increment(ix)
  #define osal_resource_monitor_decrement(ix)
#endif
/*@}*/


/**
****************************************************************************************************

  @name Enumeration of Resource Counters

  Monitored operating system resource counters are indexed. The resource monitor functions and 
  macros identify the specific resource using the resource indes.

****************************************************************************************************
*/
/*@{*/

/** Enumeration of resource counters. Specifies table index is specified for each resource which
    is monitored by OSAL.
 */
typedef enum
{
  OSAL_RMON_NONE = 0,

  /** System memory allocation. Total number of bytes allocated from operating system.
   */
  OSAL_RMON_SYSTEM_MEMORY_ALLOCATION = 1,

  /** Thread count. Number of threads created by osal_thread_create() function, but not
      terminated.
   */
  OSAL_RMON_THREAD_COUNT = 2,

  /** Event count. Number of events created by osal_event_create() function, but not deleted by
      osal_event_delete() function.
   */
  OSAL_RMON_EVENT_COUNT = 3,

  /** Event count. Number of events created by osal_event_create() function, but not deleted by
      osal_event_delete() function.
   */
  OSAL_RMON_MUTEX_COUNT = 4,

  /** File handle count. Number of currently open files.
   */
  OSAL_RMON_FILE_HANDLE_COUNT = 5,

  /** Number of open sockets.
   */
  OSAL_RMON_SOCKET_COUNT = 6,

  /** Number of open serial ports.
   */
  OSAL_RMON_SERIAL_PORT_COUNT = 7,

  /** Resource monitor table size
   */
  OSAL_RMON_COUNTERS_SZ
}
osalResourceIndex;

/*@}*/


/**
****************************************************************************************************

  @name Flags for Resource Counter Functions

  Only the osal_resource_monitor_get() function has any flags.

****************************************************************************************************
*/
/*@{*/

/** Get current value of the resource counter.
 */
#define OSAL_RMON_CURRENT 0

/** Get peak value of the resource counter.
 */
#define OSAL_RMON_PEAK 1

/*@}*/

/** 
****************************************************************************************************

  @name Resource Monitor State Structure

  The upper layers of code can monitor changes to resource use by setting a callback function.  

****************************************************************************************************
*/
typedef struct
{
	/** Current resource counter values
	 */
	os_memsz current[OSAL_RMON_COUNTERS_SZ];

	/** Peak resource counter values
	 */
	os_memsz peak[OSAL_RMON_COUNTERS_SZ];

	/** Flag indicating the changed resource counter.
	 */
	os_boolean changed[OSAL_RMON_COUNTERS_SZ];

	/** Flag indicating that some resource counter has been changed.
	 */
	os_boolean updated;
}
osalResourceMonitorState;


/** 
****************************************************************************************************

  @name Resource Monitor Functions

  These functions exists only when compiling with nonzero OSAL_DEBUG flag. Thus these
  functions are called through "debug macros".

****************************************************************************************************
 */
/*@{*/
#if OSAL_RESOURCE_MONITOR

/* Update resource counter.
 */
void osal_resource_monitor_update(
    osalResourceIndex ix,
    os_memsz delta);

/* Get resource counter value.
 */
os_long osal_resource_monitor_get_value(
    osalResourceIndex ix,
	os_int flags);

/* Check for a changed resource index.
 */
osalResourceIndex osal_resource_monitor_get_change(
	void);

#endif
/*@}*/

#endif
