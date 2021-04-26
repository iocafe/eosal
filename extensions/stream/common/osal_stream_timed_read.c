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

  @brief Write data to stream with timeout.
  @anchor osal_stream_timed_write

  The osal_stream_timed_write() function writes up to n bytes of data from buffer to socket.

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
osalStatus osal_stream_timed_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int write_timeout_ms,
    os_int flags)
{
    os_timer start_t, now_t;
    osalStatus rval;
    os_memsz n_written_now, total_written;
    os_boolean use_timer;

    if (stream)
    {
        use_timer = (os_boolean) (write_timeout_ms > 0);
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

        *n_written = total_written;
        return rval;
    }

    *n_written = 0;
    return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Read data from stream with timeout.
  @anchor osal_stream_timed_read

  The osal_stream_timed_read() function reads up to n bytes of data from socket into buffer.

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
osalStatus osal_stream_timed_read(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int read_timeout_ms,
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

    os_boolean
        use_timer;

    if (stream)
    {
        use_timer = (os_boolean) (read_timeout_ms > 0);
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

        *n_read = total_read;
        return rval;
    }

    *n_read = 0;
    return OSAL_STATUS_FAILED;
}

#endif
