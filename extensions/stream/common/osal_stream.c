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

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_FUNCTION_POINTER_SUPPORT


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
           The value OSAL_STATUS_NO_NEW_CONNECTION indicates that no new incoming
           connection, was accepted.  All other nonzero values indicate an error,
           See @ref osalStatus "OSAL function return codes" for full list.
           This parameter can be OS_NULL, if no status code is needed.
  @param   flags Flags for creating the socket. Define OSAL_STREAM_DEFAULT for normal operation.
           See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Stream pointer representing the socket, or OS_NULL if the function failed.

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
			if (rval || write_timeout_ms == 0 || n == 0) break;

			if (use_timer) 
			{
				if (n_written)
				{
                    os_get_timer(&start_t);
				}
				else
				{
					if (os_elapsed2(&start_t, &now_t, write_timeout_ms)) break;
				}
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
			if (rval || read_timeout_ms == 0 || n == 0) break;

			if (use_timer) 
			{
				if (n_read)
				{
                    os_get_timer(&start_t);
				}
				else
				{
					if (os_elapsed2(&start_t, &now_t, read_timeout_ms)) break;
				}
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
			if (rval != OSAL_STATUS_STREAM_WOULD_BLOCK || write_timeout_ms == 0) break;

			if (use_timer) 
			{
				if (os_elapsed2(&start_t, &now_t, write_timeout_ms)) break;
			}

            os_timeslice();
		}
		while (1);

		return rval;
	}
	return OSAL_STATUS_FAILED;
}

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
				if (os_elapsed2(&start_t, &now_t, read_timeout_ms)) break;
			}

            os_timeslice();
		}
		while (1);

		return rval;
	}

	*c = 0;
	return OSAL_STATUS_FAILED;
}

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

/* Return value OSAL_STATUS_NOT_SUPPORTED indicates that select is not implemented.
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
            return streams[0]->iface->stream_select(streams, nstreams, evnt, selectdata, timeout_ms, flags);
        }
        else
        {
            return OSAL_STATUS_NOT_SUPPORTED;
        }
	}

	return OSAL_STATUS_FAILED;
}

#endif
