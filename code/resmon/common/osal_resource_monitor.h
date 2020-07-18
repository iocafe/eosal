/**

  @file    osal_resource_monitor.h
  @brief   Monitor operating system resource use.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    17.7.2020

  We monitor use of operating system resources to ensure that that we will not have cumulative
  programming errors (memory leaks, etc) and that we do not transfer unnecessary data over
  communication.

  This resorce tracking code can be excluded from build by defining OSAL_RESOURCE_MONITOR as 0.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

/**
****************************************************************************************************

  @name Macros to increment or decrement resource use count

  Resource monitor macros are mapped to resource monitor functions when OSAL_RESOURCE_MONITOR
  flag is nonzero. If OSAL_RESOURCE_MONITOR flag is zero, these macros will not generate any code.

****************************************************************************************************
*/
#if OSAL_RESOURCE_MONITOR
  /** Log a programming error.
   */
  #define osal_resource_monitor_increment(ix) osal_resource_monitor_update(ix, 1)
  #define osal_resource_monitor_decrement(ix) osal_resource_monitor_update(ix, -1)
#else
  #define osal_resource_monitor_increment(ix)
  #define osal_resource_monitor_decrement(ix)
#endif


/**
****************************************************************************************************

  @name Resource counter enumeration

  Counters for monitored operating system resources are indexed.

****************************************************************************************************
*/
typedef enum
{
  OSAL_RMON_NONE = 0,

  /** System memory allocation. Total number of bytes allocated from operating system.
   */
  OSAL_RMON_SYSTEM_MEMORY_ALLOCATION ,

  /** System memory use. Number of bytes currently used trough eosal.
   */
  OSAL_RMON_SYSTEM_MEMORY_USE,

#if OSAL_MULTITHREAD_SUPPORT

  /** Thread count. Number of threads created by osal_thread_create() function, but not
      terminated.
   */
  OSAL_RMON_THREAD_COUNT,

  /** Event count. Number of events created by osal_event_create() function, but not deleted by
      osal_event_delete() function.
   */
  OSAL_RMON_EVENT_COUNT,

  /** Event count. Number of events created by osal_event_create() function, but not deleted by
      osal_event_delete() function.
   */
  OSAL_RMON_MUTEX_COUNT,

#endif
#if OSAL_FILESYS_SUPPORT

  /** File handle count. Number of currently open files.
   */
  OSAL_RMON_FILE_HANDLE_COUNT,

#endif
#if OSAL_SOCKET_SUPPORT

  /** Number of open sockets.
   */
  OSAL_RMON_SOCKET_COUNT,

  /** Number of socet connection attempt.
   */
  OSAL_RMON_SOCKET_CONNECT_COUNT,

  /** Number of bytes send trough TCP socket.
   */
  OSAL_RMON_TX_TCP,

  /** Number of bytes received trough TCP socket.
   */
  OSAL_RMON_RX_TCP,

  /** Number of bytes send trough UDP socket.
   */
  OSAL_RMON_TX_UDP,

  /** Number of bytes received trough UDP socket.
   */
  OSAL_RMON_RX_UDP,

#endif
#if OSAL_SERIAL_SUPPORT

  /** Number of bytes send trough serial port.
   */
  OSAL_RMON_TX_SERIAL,

  /** Number of bytes received trough serial port.
   */
  OSAL_RMON_RX_SERIAL,

#endif

  /** Resource monitor table size
   */
  OSAL_RMON_COUNTERS_SZ
}
osalResourceIndex;


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

  @name Resource Monitor Function

  These functions is generated only when compiling with nonzero OSAL_RESOURCE_MONITOR define.
  Otherwise the empty macro is used and no code is generated.

****************************************************************************************************
 */
#if OSAL_RESOURCE_MONITOR

/* Update resource counter.
 */
void osal_resource_monitor_update(
    osalResourceIndex ix,
    os_memsz delta);

#else

#define osal_resource_monitor_update(ix,d)

#endif

