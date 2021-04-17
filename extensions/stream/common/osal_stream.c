/**

  @file    stream/commmon/osal_stream.c
  @brief   Stream interface and interface function implementations.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    6.2.2020

  A set of intermediate functions are used to access a specific stream. For example
  application which wants to open a stream calls osal_stream_open(). A pointer to
  stream interface structure is given as argument to osal_stream_open call. This structure
  contains pointer to implementation, for example to the osal_socket_open function, so
  it can be called. The stream interface structure pointer is stored within handle, so
  it is not needed as argument for rest of the osal_stream_* functions.

  If OSAL_MINIMIMALISTIC flag is specified, only serial stream is supported (no socket, etc),
  and osal_stream_* functions map to serial functions.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"

#if OSAL_MINIMALISTIC == 0

/**
****************************************************************************************************

  @brief Open a stream (connect, listen, open file, etc).
  @anchor osal_stream_open

  The osal_stream_open() function opens a stream. The iface argument select what kind of
  stream. This function can be used to open files, connect or listen sockets, etc.

  @param  iface Stream interface, pointer to structure containing implementation function pointers.
  @param  parameters Parameter string, depends on stream type. See stream implementation
          notes. Typically all sockets take similar parameters, as do all serial ports...
  @param  option Set OS_NULL for now.
  @param  status Pointer to integer into which to store the function status code. Value
          OSAL_SUCCESS (0) indicates success and all nonzero values indicate an error.
          See @ref osalStatus "OSAL function return codes" for full list.
          This parameter can be OS_NULL, if no status code is needed.
  @param  flags Flags for creating the socket, useful flags depend on stream type.
           See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return Stream pointer (handle) representing the stream, or OS_NULL if the function failed.

****************************************************************************************************
*/
osalStream osal_stream_open(
    const osalStreamInterface *iface,
    const os_char *parameters,
    void *option,
    osalStatus *status,
    os_int flags)
{

/* ARDU TEST
 * os_char nbuf[OSAL_NBUF_SZ];
    osal_sysconsole_write(iface ? "HEHE iface\n" : "HEHE NULL\n");
osal_int_to_str(nbuf, sizeof(nbuf), (os_long)osal_serial_iface.stream_open);

osal_sysconsole_write(nbuf);
osal_sysconsole_write(" ");
osal_int_to_str(nbuf, sizeof(nbuf), (os_long)&osal_serial_open);
osal_sysconsole_write(nbuf);
osal_sysconsole_write(" ");

osal_sysconsole_write(nbuf);
osal_sysconsole_write(" ");
osal_int_to_str(nbuf, sizeof(nbuf), (os_long)iface->stream_open);
osal_sysconsole_write(nbuf);
osal_sysconsole_write("\n");
*/

    return iface->stream_open(parameters, option, status, flags);
}


/**
****************************************************************************************************

  @brief Close a stream.
  @anchor osal_stream_close

  The osal_stream_close() function a strean closes a socket, which was opened by osal_strean_open()
  function or osal_stream_accept(). All resource related to the socket are freed.

  @param   stream Stream pointer representing the socket. After this call stream pointer will
           point to invalid memory location.
  @param   flags Reserver, set OSAL_STREAM_DEFAULT (0) for now.
  @return  None.

****************************************************************************************************
*/
void osal_stream_close(
    osalStream stream,
    os_int flags)
{
    if (stream)
    {
        stream->iface->stream_close(stream, flags);
    }
}


/**
****************************************************************************************************

  @brief Accept connection to listening socket.
  @anchor osal_stream_accept

  The osal_stream_accept() function accepts an incoming connection. This function can be used
  only with TCP sockets (+TLS).

  @param   stream Stream pointer representing the listening socket.
  @param   remote_ip_address Pointer to string buffer into which to store the IP address
           from which the incoming connection was accepted. Can be OS_NULL if not needed.
  @param   remote_ip_addr_sz Size of remote IP address buffer in bytes.
  @param   status Pointer to integer into which to store the function status code. Value
           OSAL_SUCCESS (0) indicates that new connection was successfully accepted.
           The value OSAL_NO_NEW_CONNECTION indicates that no new incoming
           connection, was accepted.  All other nonzero values indicate an error,
           See @ref osalStatus "OSAL function return codes" for full list.
           This parameter can be OS_NULL, if no status code is needed.
  @param   flags Flags for opening the the socket. Define OSAL_STREAM_DEFAULT for normal operation.
           See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Stream pointer (handle) representing the stream, or OS_NULL if no new connection
           was accepted.

****************************************************************************************************
*/
osalStream osal_stream_accept(
    osalStream stream,
    os_char *remote_ip_addr,
    os_memsz remote_ip_addr_sz,
    osalStatus *status,
    os_int flags)
{
    if (stream)
    {
        return stream->iface->stream_accept(stream, remote_ip_addr,
            remote_ip_addr_sz, status, flags);
    }
    if (status) *status = OSAL_STATUS_FAILED;
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Flush writes to the stream.
  @anchor osal_stream_flush

  The osal_stream_flush() function flushes data to be written to stream.

  IMPORTANT, FLUSH MUST BE CALLED FOR SOCKETS: The osal_stream_flush(<stream>, OSAL_STREAM_DEFAULT)
  must  be called when select call returns even after writing or even if nothing was written, or
  periodically in in single thread mode. This is necessary even if no data was written
  previously, the socket may have stored buffered data to avoid blocking.

  @param   stream Stream pointer.
  @param   flags Often OSAL_STREAM_DEFAULT. See @ref osalStreamFlags "Flags for Stream Functions"
           for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_stream_flush(
    osalStream stream,
    os_int flags)
{
    if (stream)
    {
        return stream->iface->stream_flush(stream, flags);
    }
    return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Get or set file seek position.
  @anchor osal_stream_seek

  The osal_stream_seek() function is used only for files, it can be used to get or set current
  read or write position.

  @param   stream Stream pointer representing open file.
  @param   pos Position, both input and output.
  @param   flags

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_stream_seek(
    osalStream stream,
    os_long *pos,
    os_int flags)
{
    if (stream)
    {
        return stream->iface->stream_seek(stream, pos, flags);
    }
    return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Write data to stream.
  @anchor osal_stream_write

  The osal_stream_write() function writes up to n bytes of data from buffer to socket.

  Writes and reads are always non blocking. Blocking behaviour can be emulated by setting
  nonzero read and write timeouts. This is important for sockets and serial ports.
  For many stream types, like files, stream buffers, etc, either succeed or fail immediately.

  @param   stream Stream pointer.
  @param   buf Pointer to the beginning of data to write to stream.
  @param   n Maximum number of bytes to write.
  @param   n_written Pointer to integer into which the function stores the number of bytes
           actually written, which may be less than n if there is not enough space
           left in the buffer. If the function fails n_written is set to zero.
  @param   flags Flags for the function.
           See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_stream_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags)
{
    os_timer start_t, now_t;
    osalStatus rval;
    os_memsz n_written_now, total_written;
    os_int write_timeout_ms;
    os_boolean use_timer;

    if (stream)
    {
        write_timeout_ms = stream->write_timeout_ms;
        use_timer = (os_boolean) ((flags & OSAL_STREAM_WAIT) != 0 && write_timeout_ms > 0);
        if (use_timer) os_get_timer(&start_t);
        total_written = 0;
        do
        {
            if (use_timer) os_get_timer(&now_t);
            rval = stream->iface->stream_write(stream, buf, n, &n_written_now, flags);
            total_written += n_written_now;
            n -= n_written_now;
            if (rval || !use_timer || n == 0) break;

            if (n_written_now) {
                os_get_timer(&start_t);
            }
            else {
                if (os_has_elapsed_since(&start_t, &now_t, write_timeout_ms)) break;
            }

            buf += n_written_now;
            os_timeslice();
        }
        while (1);

        if (n_written) *n_written = total_written;
        return rval;
    }

    if (n_written) *n_written = 0;
    return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Read data from stream.
  @anchor osal_stream_read

  The osal_stream_read() function reads up to n bytes of data from socket into buffer.

  @param   stream Stream pointer.
  @param   buf Pointer to buffer to read into.
  @param   n Maximum number of bytes to read. The data buffer must large enough to hold
           at least this many bytes.
  @param   n_read Pointer to integer into which the function stores the number of bytes read,
           which may be less than n if there are fewer bytes available. If the function fails
           n_read is set to zero.
  @param   flags Flags for the function, use OSAL_STREAM_DEFAULT (0) for default operation.

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_stream_read(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags)
{
    os_timer
        start_t,
        now_t;

    osalStatus
        rval;

    os_memsz
        n_read_now,
        total_read;

    os_int
        read_timeout_ms;

    os_boolean
        use_timer;

    if (stream)
    {
        read_timeout_ms = stream->read_timeout_ms;
        use_timer = (os_boolean) ((flags & OSAL_STREAM_WAIT) != 0 && read_timeout_ms > 0);
        if (use_timer) os_get_timer(&start_t);
        total_read = 0;
        do
        {
            if (use_timer) os_get_timer(&now_t);
            rval = stream->iface->stream_read(stream, buf, n, &n_read_now, flags);
            total_read += n_read_now;
            n -= n_read_now;
            if (rval || !use_timer || n == 0) break;

            if (n_read_now) {
                os_get_timer(&start_t);
            }
            else {
                if (os_has_elapsed_since(&start_t, &now_t, read_timeout_ms)) break;
            }

            buf += n_read_now;
            os_timeslice();
        }
        while (1);

        if (n_read) *n_read = total_read;
        return rval;
    }

    if (n_read) *n_read = 0;
    return OSAL_STATUS_FAILED;
}


#if OSAL_MICROCONTROLLER == 0
/**
****************************************************************************************************

  @brief Write single value.
  @anchor osal_stream_write_value

  The osal_stream_write_value() function writes one byte value or control code to stream.
  This function is used by eobjects for serialized object versioning, do not use from iocom, etc.

  @param   stream Stream pointer.
  @param   c Byte or control code to write.
  @param   flags Flags for the function, use OSAL_STREAM_DEFAULT (0) for default operation.

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_stream_write_value(
    osalStream stream,
    os_ushort c,
    os_int flags)
{
    os_timer
        start_t,
        now_t;

    osalStatus
        rval;

    os_int
        write_timeout_ms;

    os_boolean
        use_timer;

    if (stream)
    {
        write_timeout_ms = stream->write_timeout_ms;
        use_timer = (os_boolean) ((flags & OSAL_STREAM_WAIT) != 0 && write_timeout_ms > 0);
        if (use_timer) os_get_timer(&start_t);
        do
        {
            if (use_timer) os_get_timer(&now_t);
            rval = stream->iface->stream_write_value(stream, c, flags);
            if (rval != OSAL_STATUS_STREAM_WOULD_BLOCK || !use_timer) break;

            if (use_timer)
            {
                if (os_has_elapsed_since(&start_t, &now_t, write_timeout_ms)) break;
            }

            os_timeslice();
        }
        while (1);

        return rval;
    }
    return OSAL_STATUS_FAILED;
}
#endif


#if OSAL_MICROCONTROLLER == 0
/**
****************************************************************************************************

  @brief Read single value.
  @anchor osal_stream_read_value

  The osal_stream_read_value() function reads one byte value or control code to stream.
  This function is used by eobjects for serialized object versioning, do not use from iocom, etc.

  @param   stream Stream pointer.
  @param   c Byte or control code to write.
  @param   flags Flags for the function, use OSAL_STREAM_DEFAULT (0) for default operation.

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_stream_read_value(
    osalStream stream,
    os_ushort *c,
    os_int flags)
{
    os_timer start_t, now_t;
    osalStatus rval;
    os_int read_timeout_ms;
    os_boolean use_timer;

    if (stream)
    {
        read_timeout_ms = stream->read_timeout_ms;
        use_timer = (os_boolean) ((flags & OSAL_STREAM_WAIT) != 0 && read_timeout_ms > 0);
        if (use_timer) os_get_timer(&start_t);
        do
        {
            if (use_timer) os_get_timer(&now_t);
            rval = stream->iface->stream_read_value(stream, c, flags);
            if (rval != OSAL_STATUS_STREAM_WOULD_BLOCK || read_timeout_ms == 0) break;

            if (use_timer)
            {
                if (os_has_elapsed_since(&start_t, &now_t, read_timeout_ms)) break;
            }

            os_timeslice();
        }
        while (1);

        return rval;
    }

    *c = 0;
    return OSAL_STATUS_FAILED;
}
#endif


/**
****************************************************************************************************

  @brief Get stream parameter.
  @anchor osal_stream_get_parameter

  The osal_stream_get_parameter() function gets stream parameter value.

  @param   stream Stream pointer representing the serial.
  @param   parameter_ix Index of parameter to get.
           See @ref osalStreamParameterIx "stream parameter enumeration" for the list.
  @return  Parameter value.

****************************************************************************************************
*/
os_long osal_stream_get_parameter(
    osalStream stream,
    osalStreamParameterIx parameter_ix)
{
    if (stream)
    {
        return stream->iface->stream_get_parameter(stream, parameter_ix);
    }
    return 0;
}


/**
****************************************************************************************************

  @brief Set stream parameter.
  @anchor osal_stream_set_parameter

  The osal_stream_set_parameter() function sets a stream parameter.

  @param   stream Stream pointer representing the serial.
  @param   parameter_ix Index of parameter to set.
           See @ref osalStreamParameterIx "stream parameter enumeration" for the list.
  @param   value Parameter value.
  @return  None.

****************************************************************************************************
*/
void osal_stream_set_parameter(
    osalStream stream,
    osalStreamParameterIx parameter_ix,
    os_long value)
{
    if (stream)
    {
        stream->iface->stream_set_parameter(stream, parameter_ix, value);
    }
}


/**
****************************************************************************************************

  @brief Block thread until something is received from stream or event occurs.
  @anchor osal_stream_select

  The osal_stream_select() function blocks execution of the calling thread until something
  is received from stream, we can write data to it, or event given as argument is triggered.

  @param   streams Array of streams to wait for. These must be serial ports, no mixing
           of different stream types is supported.
  @param   n_streams Number of stream pointers in "streams" array.
  @param   evnt Custom event to interrupt the select. OS_NULL if not needed.
  @param   selectdata Pointer to structure to fill in with information why select call
           returned. The "stream_nr" member is stream number which triggered the return:
           0 = first stream, 1 = second stream... Or one of OSAL_STREAM_NR_CUSTOM_EVENT,
           OSAL_STREAM_NR_TIMEOUT_EVENT or OSAL_STREAM_NR_UNKNOWN_EVENT. These indicate
           that event was triggered, wait timeout, and that stream implementation did
           not provide reason.
  @param   timeout_ms Maximum time to wait in select, ms. If zero, timeout is not used (infinite).
  @param   flags Ignored, set OSAL_STREAM_DEFAULT (0).
  @return  If successful, the function returns OSAL_SUCCESS and the selectdata tells which
           socket or event triggered the thread to continue. Return value OSAL_STATUS_NOT_SUPPORTED
           indicates that select is not implemented. Other return values indicate an error.
           See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_stream_select(
    osalStream *streams,
    os_int nstreams,
    osalEvent evnt,
    osalSelectData *selectdata,
    os_int timeout_ms,
    os_int flags)
{
    if (nstreams) if (streams[0])
    {
        if (streams[0]->iface->stream_select)
        {
            return streams[0]->iface->stream_select(streams, nstreams,
                evnt, selectdata, timeout_ms, flags);
        }
        else
        {
            return OSAL_STATUS_NOT_SUPPORTED;
        }
    }

    return OSAL_STATUS_FAILED;
}


/* Write packet (UDP) to stream.
 */
osalStatus osal_stream_send_packet(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_int flags)
{
    if (stream) if (stream->iface->stream_send_packet)
    {
        return stream->iface->stream_send_packet(stream, buf, n, flags);
    }
    return OSAL_STATUS_NOT_SUPPORTED;
}

/* Read packet (UDP) from stream.
 */
osalStatus osal_stream_receive_packet(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_char *remote_addr,
    os_memsz remote_addr_sz,
    os_int flags)
{
    if (stream) if (stream->iface->stream_receive_packet)
    {
        return stream->iface->stream_receive_packet(stream, buf, n, n_read,
            remote_addr, remote_addr_sz, flags);
    }
    return OSAL_STATUS_NOT_SUPPORTED;
}

#endif
