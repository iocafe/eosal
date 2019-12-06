/**

  @file    defs/common/osal_status.h
  @brief   OSAL function return codes.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Many OSAL functions return status codes, which are enumerated here.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_STATUS_INCLUDED
#define OSAL_STATUS_INCLUDED


/**
****************************************************************************************************

  @name Function return codes
  @anchor osalStatus

  Many OSAL function returns status code. Zero is always success and other values identify
  an error or an exception condition.

****************************************************************************************************
*/
/*@{*/

/** Status codes
 */
typedef enum
{
    /** Success.
     */
    OSAL_SUCCESS = 0,

    /** No work to be done (not indicating an error).
     */
    OSAL_STATUS_NOTHING_TO_DO,

    /** General operation pending (not indicating an error).
     */
    OSAL_STATUS_PENDING,

    /** Indicating that we are dealing with IP v6 address (not indicating an error).
     */
    OSAL_STATUS_IS_IPV6,

    /** General failed.
     */
    OSAL_STATUS_FAILED,

    /** General time out. Among other things, used by osal_event_wait() function. If event
        doesn't get signaled before timeout interval given as argument, this code is returned.
     */
    OSAL_STATUS_TIMEOUT,

    /** Operation not supported for this operating system/hardware platform.
     */
    OSAL_STATUS_NOT_SUPPORTED,

    /** Attempt to call interface function which is not implemented.
     */
    OSAL_STATUS_NULL_FUNC,

    /** Creating thread failed.
     */
    OSAL_STATUS_THREAD_CREATE_FAILED,

    /** Setting thread priority failed.
     */
    OSAL_STATUS_THREAD_SET_PRIORITY_FAILED,

    /** Creating an event failed.
     */
    OSAL_STATUS_EVENT_CREATE_EVENT_FAILED,

    /** General failure code for osal_event.c. This indicates a programming error.
     */
    OSAL_STATUS_EVENT_FAILED,

    /** Memory allocation from operating system has failed.
     */
    OSAL_STATUS_MEMORY_ALLOCATION_FAILED,

    /** Setting computer's clock failed.
     */
    OSAL_STATUS_CLOCK_SET_FAILED,

	/** Call would block. The stream functions osal_stream_read_value() and 
	    osal_stream_write_value() return this code to indicate that no data was received
		or sent, because it would otherwise block the calling thread. 
	 */
	OSAL_STATUS_STREAM_WOULD_BLOCK,

	/** No new incoming connection. The stream function osal_stream_accept() return this 
	    code to indicate that no new connection was accepted. 
	 */
    OSAL_STATUS_NO_NEW_CONNECTION,

    /** Socket connection has been refused by server. Propably service not running.
     */
    OSAL_STATUS_CONNECTION_REFUSED,

    /** Socket or other stream has been closed.
     */
    OSAL_STATUS_STREAM_CLOSED,

    /** Process does not have access right to object.
     */
    OSAL_STATUS_NO_ACCESS_RIGHT,

    /** Device is out of free space.
     */
    OSAL_STATUS_DISC_FULL,

    /** Attempt to open file which doesn't exist.
     */
    OSAL_FILE_DOES_NOT_EXIST,

    /** Attempt to open file which doesn't exist.
     */
    OSAL_DIR_NOT_EMPTY,

    /** End of file has been reached.
     */
    OSAL_END_OF_FILE,

    /** Run out of user given buffer.
     */
    OSAL_OUT_OF_BUFFER,

    /** Check sum doesn't match.
     */
    OSAL_CHECKSUM_ERROR,

    /** Handle has been closed (object referred by handle doesn't exist)
     */
    OSAL_STATUS_HANDLE_CLOSED
}
osalStatus;

/*@}*/



#endif
