/**

  @file    stream/common/osal_stream.h
  @brief   Stream interface for OSAL stream API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    6.2.2020

  Definition OSAL stream API: defines osalStreamInterface structure. function prototypes, etc.
  The OSAL stream API is abstraction which makes all streams (including sockets) look more or less
  similar to upper levels of code, regardless of operating system, network library, or other
  transport actually used.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_STREAM_H_
#define OSAL_STREAM_H_
#include "eosalx.h"

/**
****************************************************************************************************

  @name Stream Pointer Type

  The osalStream type is pointer to a stream, like stream handle. It is pointer to stream's
  internal data structure, which holds the state of the stream. The stream's structure will
  start with the stream header structure osalStreamHeader, which contains information common
  to all streams. OSAL functions cast their own stream pointers to osalStream pointers and back.

****************************************************************************************************
*/
/* Declare stream header structure, defined later in this file.
 */
struct osalStreamHeader;

/** Stream pointer returned by osal_stream_open() function.
 */
typedef struct osalStreamHeader *osalStream;

/** Timeout to wait for forever.
 */
#define OSAL_INFINITE -1


/**
****************************************************************************************************

  @name Flags for Stream Functions.
  @anchor osalStreamFlags

  These flags nodify how stream functions behave. Some flags are appropriate for many functions,
  and some flags are effect only one.

  Note: bit 0x0100000 and larger are reserved to eStream

****************************************************************************************************
*/
/** No special flags. The OSAL_STREAM_DEFAULT (0) can be given as flag for any stream function,
    which takes flags. It simply specifies no special flags.
 */
#define OSAL_STREAM_DEFAULT 0

/** Open stream for reading. The OSAL_STREAM_READ flag is significant only for osal_stream_open()
    function. To open stream for both reading and writing, use OSAL_STREAM_RW.
 */
#define OSAL_STREAM_READ 0x0001

/** Open stream for writing. The OSAL_STREAM_WRITE flag is significant only for osal_stream_open()
    function. To open stream for both reading and writing, use OSAL_STREAM_RW.
 */
#define OSAL_STREAM_WRITE 0x0002

/** Open stream for both reading and writing. The OSAL_STREAM_RW flag is significant only for
    osal_stream_open() function. It simply combines OSAL_STREAM_READ and OSAL_STREAM_WRITE flags.
 */
#define OSAL_STREAM_RW (OSAL_STREAM_READ|OSAL_STREAM_WRITE)

/** Open stream for appending. The OSAL_STREAM_APPEND flag is significant only for
    osal_stream_open() function when opening a file. Current file content is preserved
    and file pointer is set at end of file.
 */
#define OSAL_STREAM_APPEND 0x0004

/** oestream_flush, clear receive buffer.
 */
#define OSAL_STREAM_CLEAR_RECEIVE_BUFFER 0x0010

/** oestream_flush, clear transmit buffer.
 */
#define OSAL_STREAM_CLEAR_TRANSMIT_BUFFER 0x0020

/** Open a socket to to connect. Connect is default socket operation, OSAL_STREAM_CONNECT
    is zero
 */
#define OSAL_STREAM_CONNECT 0

/** Open a socket to listen for incoming connections.
 */
#define OSAL_STREAM_LISTEN 0x0100

/** Peek into stream buffer, do not empty receive buffer. Implemented only for some streams.
 */
#define OSAL_STREAM_PEEK 0x0200

/** Open a UDP multicast socket.
 */
#define OSAL_STREAM_MULTICAST 0x0400

/** Open socket without select functionality.
 */
#define OSAL_STREAM_NO_SELECT 0x0800

/** Open socket with select functionality.
 */
#define OSAL_STREAM_SELECT 0x0000

/** Disable Nagle's algorithm on TCP socket. Use TCP_CORK on linux, or TCP_NODELAY
    toggling on windows. If this flag is set, osal_stream_flush() must be called
    to actually transfer data.
 */
#define OSAL_STREAM_TCP_NODELAY 0x1000

/** Disable reusability of the socket descriptor.
 */
#define OSAL_STREAM_NO_REUSEADDR 0x2000

/** We access write position (seek), not read position.
 */
#define OSAL_STREAM_SEEK_WRITE_POS 0x8000

/** Set seek position, either read or write (depends on OSAL_STREAM_SEEK_WRITE_POS flag).
 */
#define OSAL_STREAM_SEEK_SET 0x10000

/** This flag can be given to osal_stream_close (some streams only) to indicate
    that transferring data, etc, has failed and transferred content should be ignored.
 */
#define OSAL_STREAM_INTERRUPT 0x10000

/** In special case (streaming over iocom memory block) this value fill tell
    stream that transfer is written completed successful and final handshake
    needs to take place.
 */
#define OSAL_STREAM_FINAL_HANDSHAKE 0x20000

/** Enable use of global settings. This is used for global UDP multicast addressess by lighthouse
    discovery protocol.
 */
#define OSAL_STREAM_USE_GLOBAL_SETTINGS 0x40000

/** Disable checksum calculation on the stream (iocom streamer only).
 */
#define OSAL_STREAM_DISABLE_CHECKSUM 0x80000


/**
****************************************************************************************************
  Stream flags reserved for eObjects library.
****************************************************************************************************
*/

/** eQueue specific flag: Encode data when writing into queue. If not set, data is written to
    queue as is.
 */
#define OSAL_STREAM_ENCODE_ON_WRITE 0x0100000

/** eQueue specific flag: Decode data when reading from queue. If not set, data is read from
    queue as is.
 */
#define OSAL_STREAM_DECODE_ON_READ 0x0200000

/** eQueue specific flag: Maintain flush control count within queue when reading input
    from socket, etc.
 */
#define OSAL_FLUSH_CTRL_COUNT 0x0400000


/**
****************************************************************************************************

  @name Stream interface flags (iflags).

  These flags pass general information about the stream, like is this secure TLS stream, which
  can be used for transferring security data (passwords, etc).

****************************************************************************************************
*/
/** No interface flags, define used instead of 0 to allow string search.
 */
#define OSAL_STREAM_IFLAG_NONE 0

/** Secured transport (TLS).
 */
#define OSAL_STREAM_IFLAG_SECURE 1


/**
****************************************************************************************************

  Stream Interface structure.

  The interface structure contains set of function pointers. These function pointers point
  generally to functions which do implemen a specific stream. The functions pointer can also
  point to default implementations in osal_stream.c.

****************************************************************************************************
*/
typedef struct osalStreamInterface
{
    /* Stream interface flags.
     */
    os_int iflags;

    /* Open a stream (connect, listen, open file, etc).
     */
    osalStream (*stream_open)(
        const os_char *parameters,
        void *option,
        osalStatus *status,
        os_int flags);

    /* Close an open stream.
     */
    void (*stream_close)(
        osalStream stream,
        os_int flags);

    /* Accept incoming socket.
     */
    osalStream (*stream_accept)(
        osalStream stream,
        os_char *remote_ip_addr,
        os_memsz remote_ip_addr_sz,
        osalStatus *status,
        os_int flags);

    /* Flush writes to stream (call even nothing was written to flush buffering)
     */
    osalStatus (*stream_flush)(
        osalStream stream,
        os_int flags);

    /* Set seek position. This can be used for files.
     */
    osalStatus (*stream_seek)(
        osalStream stream,
        os_long *pos,
        os_int flags);

    /* Write data to stream.
     */
    osalStatus (*stream_write)(
        osalStream stream,
        const os_char *buf,
        os_memsz n,
        os_memsz *n_written,
        os_int flags);

    /* Read data from stream.
     */
    osalStatus (*stream_read)(
        osalStream stream,
        os_char *buf,
        os_memsz n,
        os_memsz *n_read,
        os_int flags);

    /* Block thread until something is received from stream or event occurs.
     */
    osalStatus (*stream_select)(
        osalStream *streams,
        os_int nstreams,
        osalEvent evnt,
        os_int timeout_ms,
        os_int flags);

    /* Write packet (UDP) to stream.
     */
    osalStatus (*stream_send_packet)(
        osalStream stream,
        const os_char *buf,
        os_memsz n,
        os_int flags);

    /* Read packet (UDP) from stream.
     */
    osalStatus (*stream_receive_packet)(
        osalStream stream,
        os_char *buf,
        os_memsz n,
        os_memsz *n_read,
        os_char *remote_addr,
        os_memsz remote_addr_sz,
        os_int flags);
}
osalStreamInterface;


/**
****************************************************************************************************

  Stream header structure.
  @anchor osalStreamHeader

  All stream class structures start with osalStream header, this is the generic portion of
  data for a stream object. Type osalStream used to as stream handle is actually pointer
  to osalStreamHeader.

****************************************************************************************************
*/
typedef struct osalStreamHeader
{
    /** Pointer to stream interface is always first item of the handle
     */
    const osalStreamInterface *iface;
}
osalStreamHeader;


/**
****************************************************************************************************

  @name Stream Functions

  These functions access the underlying stream implementation trough iface pointer in
  stream data structure's osalStreamHeader member. The functions add extra capabilities
  to streams, most importantly support for OSAL_STREAM_WAIT flag.

****************************************************************************************************
*/
#if OSAL_MINIMALISTIC == 0

/* Open a stream (connect, listen, open file, etc).
 */
osalStream osal_stream_open(
    const osalStreamInterface *iface,
    const os_char *parameters,
    void *option,
    osalStatus *status,
    os_int flags);

/* Close a stream.
 */
void osal_stream_close(
    osalStream stream,
    os_int flags);

/* Accept an incoming socket.
 */
osalStream osal_stream_accept(
    osalStream stream,
    os_char *remote_ip_addr,
    os_memsz remote_ip_addr_sz,
    osalStatus *status,
    os_int flags);

/* Flush writes to stream (call even nothing was written to flush buffering)
 */
osalStatus osal_stream_flush(
    osalStream stream,
    os_int flags);

/* Set seek position. This can be used for files.
 */
osalStatus osal_stream_seek(
    osalStream stream,
    os_long *pos,
    os_int flags);

/* Write data to stream
 */
osalStatus osal_stream_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags);

/* Read data from stream
 */
osalStatus osal_stream_read(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags);

/* Block thread until something is received from stream or event occurs.
 */
osalStatus osal_stream_select(
    osalStream *streams,
    os_int nstreams,
    osalEvent evnt,
    os_int timeout_ms,
    os_int flags);

/* Write packet (UDP) to stream.
 */
osalStatus osal_stream_send_packet(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_int flags);

/* Read packet (UDP) from stream.
 */
osalStatus osal_stream_receive_packet(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_char *remote_addr,
    os_memsz remote_addr_sz,
    os_int flags);

osalStatus osal_stream_timed_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int write_timeout_ms,
    os_int flags);

osalStatus osal_stream_timed_read(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int read_timeout_ms,
    os_int flags);

#endif

/**
****************************************************************************************************

  @name Macros to map stream functions directly to serial port

  If OSAL_MINIMALISTIC is set, these macros fix stream interface to serial port.
  Serial port will be the only supported stream.

****************************************************************************************************
*/

#if OSAL_MINIMALISTIC
#define osal_stream_open(i,p,o,s,f) osal_serial_open(p,o,s,f)
#define osal_stream_close(s,f) osal_serial_close(s,f)
#define osal_stream_accept(s,a,z,ss,f) osal_stream_default_accept(s,a,z,ss,f)
#define osal_stream_flush(s,f) osal_serial_flush(s,f)
#define osal_stream_seek(s,p,f) osal_stream_default_seek(s,p,f)
#define osal_stream_write(s,b,n,nn,f) osal_serial_write(s,b,n,nn,f)
#define osal_stream_read(s,b,n,nn,f) osal_serial_read(s,b,n,nn,f)
#define osal_stream_select(s,n,e,t,f) osal_stream_default_select(s,n,e,t,f)
#define osal_stream_send_packet(s,b,n,f) OSAL_STATUS_NOT_SUPPORTED
#define osal_stream_receive_packet(s,b,n,nn,a,z,f) OSAL_STATUS_NOT_SUPPORTED
#endif

#endif
