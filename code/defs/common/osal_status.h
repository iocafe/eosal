/**

  @file    defs/common/osal_status.h
  @brief   OSAL function return codes.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Many OSAL functions return status codes, which are enumerated here.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

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
    OSAL_NOTHING_TO_DO = 2,

    /** General operation pending (not indicating an error).
     */
    OSAL_PENDING = 4,

    /** General operation completed (not indicating an error).
     */
    OSAL_COMPLETED = 6,

    /** General "memory has been allocated" status (not indicating an error).
     */
    OSAL_MEMORY_ALLOCATED = 8,

    /** Indicating that we are dealing with IP v6 address (not indicating an error).
     */
    OSAL_IS_IPV6 = 10,

    /** Information (not indicating an error).
     */
    OSAL_SOCKET_CONNECTED = 20,

    /** No new incoming connection. The stream function osal_stream_accept() return this
        code to indicate that no new connection was accepted.
     */
    OSAL_NO_NEW_CONNECTION = 21,

    /** Information (not indicating an error).
     */
    OSAL_SOCKET_DISCONNECTED = 22,

    /** Information (not indicating an error).
     */
    OSAL_LISTENING_SOCKET_CONNECTED = 24,

    /** Information (not indicating an error).
     */
    OSAL_LISTENING_SOCKET_DISCONNECTED = 26,

    /** Information (not indicating an error).
     */
    OSAL_UDP_SOCKET_CONNECTED = 28,

    /** Information (not indicating an error).
     */
    OSAL_UDP_SOCKET_DISCONNECTED = 30,

    /** IO device has been assigned to IO network. This is feature of light house, so that
        IO devices do not need to be pre configured to network, if for example running in
        internal network of small automated system.
     */
    // OSAL_IO_NETWORK_NAME_SET = 40,

    /** End of file has been reached.
     */
    OSAL_END_OF_FILE = 50,

    /** General failed, start enumerating errors from 100.
     */
    OSAL_STATUS_FAILED = 100,

    /** Object or software library has not been initialized.
     */
    OSAL_STATUS_NOT_INITIALIZED = 102,

    /** General, something is not connected.
     */
    OSAL_STATUS_NOT_CONNECTED = 103,

    /** General time out. Among other things, used by osal_event_wait() function. If event
        doesn't get signaled before timeout interval given as argument, this code is returned.
     */
    OSAL_STATUS_TIMEOUT = 104,

    /** Certificate presented by TLS server has been rejected by client.
     */
    OSAL_STATUS_SERVER_CERT_REJECTED = 106,

    /** TLS certificate or key could not be loaded.
     */
    OSAL_STATUS_CERT_OR_KEY_NOT_AVAILABLE = 108,

    /** TLS certificate or key could not be parsed, corrupted?
     */
    OSAL_STATUS_PARSING_CERT_OR_KEY_FAILED = 109,

    /** Operation not authorized by security.
     */
    OSAL_STATUS_NOT_AUTOHORIZED = 110,

    /** Operation not supported for this operating system/hardware platform.
     */
    OSAL_STATUS_NOT_SUPPORTED = 120,

    /** Creating thread failed.
     */
    OSAL_STATUS_THREAD_CREATE_FAILED = 130,

    /** Setting thread priority failed.
     */
    OSAL_STATUS_THREAD_SET_PRIORITY_FAILED = 132,

    /** Creating an event failed.
     */
    OSAL_STATUS_EVENT_CREATE_EVENT_FAILED  = 134,

    /** General failure code for osal_event.c. This indicates a programming error.
     */
    OSAL_STATUS_EVENT_FAILED = 136,

    /** Memory allocation from operating system has failed.
     */
    OSAL_STATUS_MEMORY_ALLOCATION_FAILED = 138,

    /** Unable to read file, persisten block, etc.
     */
    OSAL_STATUS_READING_FILE_FAILED = 140,

    /** Unable to write file, persisten block, etc.
     */
    OSAL_STATUS_WRITING_FILE_FAILED = 142,

    /** Setting computer's clock failed.
     */
    OSAL_STATUS_CLOCK_SET_FAILED = 150,

	/** Call would block. The stream functions osal_stream_read_value() and 
	    osal_stream_write_value() return this code to indicate that no data was received
		or sent, because it would otherwise block the calling thread. 
	 */
    OSAL_STATUS_STREAM_WOULD_BLOCK = 160,

    /** Writing program to device has failed.
     */
    OSAL_DEVICE_PROGRAMMING_FAILED = 166,

    /** Program installation has failed.
     */
    OSAL_STATUS_PROGRAM_INSTALLATION_FAILED = 168,

    /** Not connected to a WiFi network.
     */
    OSAL_STATUS_NO_WIFI = 170,

    /** UDP socket open failed (typically used for UDP multicasts).
     */
    OSAL_STATUS_OPENING_UDP_SOCKET_FAILED = 172,

    /** Sending UDP packet (usually multicast) failed.
     */
    OSAL_STATUS_SEND_MULTICAST_FAILED = 174,

    /** Receiving UDP packet (usually multicast) failed.
     */
    OSAL_STATUS_RECEIVE_MULTICAST_FAILED = 176,

    /** Joining UDP multicast group failed.
     */
    OSAL_STATUS_MULTICAST_GROUP_FAILED = 178,

    /** Selecting network interface to use for multicast failed.
     */
    OSAL_STATUS_SELECT_MULTICAST_IFACE_FAILED = 180,

    /** UDP multicast received by "lighthouse" was not understood.
     */
    OSAL_STATUS_UNKNOWN_LIGHTHOUSE_MULTICAST = 182,

    /** Socket connection has been refused by server.
     */
    OSAL_STATUS_CONNECTION_REFUSED = 184,

    /** Socket connection has been been reseted.
     */
    OSAL_STATUS_CONNECTION_RESET = 185,

    /** Process does not have access right to object.
     */
    OSAL_STATUS_NO_ACCESS_RIGHT = 186,

    /** Creating new process has failed.
     */
    OSAL_STATUS_CREATE_PROCESS_FAILED = 188,

    /** Socket or other stream has been closed.
     */
    OSAL_STATUS_STREAM_CLOSED = 190,

    /** Device is out of free space.
     */
    OSAL_STATUS_DISC_FULL = 192,

    /** Attempt to open file which doesn't exist.
     */
    OSAL_STATUS_FILE_DOES_NOT_EXIST = 194,

    /** Attempt to open file which doesn't exist.
     */
    OSAL_STATUS_DIR_NOT_EMPTY = 196,

    /** Run out of user given buffer.
     */
    OSAL_STATUS_OUT_OF_BUFFER = 202,

    /** Check sum doesn't match.
     */
    OSAL_STATUS_CHECKSUM_ERROR = 204,

    /** Handle has been closed (object referred by handle doesn't exist)
     */
    OSAL_STATUS_HANDLE_CLOSED = 206
}
osalStatus;

/* Macro to determine if returned status is error.
 */
#define OSAL_IS_ERROR(s) ((s) >= OSAL_STATUS_FAILED)

/*@}*/
