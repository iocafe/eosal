/**

  @file    stream/commmon/osal_stream.c
  @brief   Stream interface and default function implementations.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Stream interface implementation.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"

#if OSAL_FUNCTION_POINTER_SUPPORT

osalStream osal_stream_open(
	osalStreamInterface *iface,
    const os_char *parameters,
	void *option,
	osalStatus *status,
	os_int flags)
{
	return iface->stream_open(parameters, option, status, flags);
}

void osal_stream_close(
	osalStream stream)
{
	if (stream)
	{
		stream->iface->stream_close(stream);
	}
}

osalStream osal_stream_accept(
	osalStream stream,
	osalStatus *status,
	os_int flags)
{
	if (stream)
	{
		return stream->iface->stream_accept(stream, status, flags);
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
	const os_uchar *buf,
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
	os_uchar *buf,
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
        return streams[0]->iface->stream_select(streams, nstreams, evnt, selectdata, timeout_ms, flags);
	}

	return OSAL_STATUS_FAILED;
}

#endif


osalStream osal_stream_default_accept(
	osalStream stream,
	osalStatus *status,
	os_int flags)
{
	if (status) *status = OSAL_STATUS_FAILED;
	return OS_NULL;
}


osalStatus osal_stream_default_seek(
	osalStream stream,
	os_long *pos,
	os_int flags)
{
	return OSAL_STATUS_FAILED;
}


osalStatus osal_stream_default_write_value(
	osalStream stream,
	os_ushort c,
	os_int flags)
{
	os_uchar 
		u;

	os_memsz 
		n_written;

	osalStatus 
		status;

	u = (os_uchar)c;

	status = stream->iface->stream_write(stream, &u, 1, &n_written, flags);
	if (status) return status;

	return n_written ? OSAL_SUCCESS : OSAL_STATUS_STREAM_WOULD_BLOCK;
}


osalStatus osal_stream_default_read_value(
	osalStream stream,
	os_ushort *c,
	os_int flags)
{
	os_uchar 
		u = 0;

	os_memsz 
		n_read;

	osalStatus 
		status;

	status = stream->iface->stream_read(stream, &u, 1, &n_read, flags);
	*c = u;

	if (status) return status;
	return n_read ? OSAL_SUCCESS : OSAL_STATUS_STREAM_WOULD_BLOCK;
}


os_long osal_stream_default_get_parameter(
	osalStream stream,
	osalStreamParameterIx parameter_ix)
{
    os_long value = -1;

    if (stream) switch (parameter_ix)
    {
        case OSAL_STREAM_WRITE_TIMEOUT_MS:
            value = stream->write_timeout_ms;
            break;

        case OSAL_STREAM_READ_TIMEOUT_MS:
            value = stream->read_timeout_ms;
            break;

        default:
            break;
    }

    return value;
}

void osal_stream_default_set_parameter(
	osalStream stream,
	osalStreamParameterIx parameter_ix,
	os_long value)
{
    if (stream) switch (parameter_ix)
    {
        case OSAL_STREAM_WRITE_TIMEOUT_MS:
            stream->write_timeout_ms = (os_int)value;
            break;

        case OSAL_STREAM_READ_TIMEOUT_MS:
            stream->read_timeout_ms = (os_int)value;
            break;

        default:
            break;
    }
}

osalStatus osal_stream_default_select(
	osalStream *streams,
    os_int nstreams,
	osalEvent evnt,
	osalSelectData *selectdata,
    os_int timeout_ms,
	os_int flags)
{
	return OSAL_STATUS_FAILED;
}
