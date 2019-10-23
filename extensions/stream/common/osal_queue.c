/**

  @file    stream/queue/osal_queue.c
  @brief   Byte queues.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Byte queue implementation. A byte queue is a stream which can be written to and read from.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if 0

/** Number of bytes of queue buffer to reserve for control codes only.
 */
#define OSAL_QUEUE_CTRL_RESERVE 64

/** Byte value to mark control code.
 */
#define OSAL_QUEUE_CTRL_CODE 0xE9

/**
****************************************************************************************************

  @brief Construct a byte queue.
  @anchor osal_queue_open

  The osal_queue_open() function allocates and sets up byte queue. The queue is a stream buffer 
  which can be written to and read from. 
  The queue is not by default internally synchronized by mutex, but it is written so that if only
  one thread writes to queue and only one thread reads to queue, syncronization is not necessary.
  If this is not the case, flag OSAL_STREAM_SYNCHRONIZE will force queue access to be fully 
  synchronized. This has small performance penalty, but allows any number of threads to
  write to queue.

  @param   parameters Queue parameters, a list string. "buf=1024" sets minimum usable queue buffer 
		   size to 1024 bytes (default is 512 bytes). If parameters are not needed this argument
		   can be OS_NULL or empty string.

  @param   option Not used for byte queues, set OS_NULL.

  @param   status Pointer to integer into which to store the function status code. Value
		   OSAL_SUCCESS (0) indicates success and all nonzero values indicate an error.
           See @ref osalStatus "OSAL function return codes" for full list.
		   This parameter can be OS_NULL, if no status code is needed. 

  @param   flags Flags for creating the queue. Define OSAL_STREAM_DEFAULT for normal operation.
		   flag OSAL_STREAM_SYNCHRONIZE specifies full thread synchzonization. This flag can
		   be used if the queue is used by multiple threads.
		   See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.

  @return  Stream pointer representing the byte queue, or OS_NULL if the function failed.

****************************************************************************************************
*/
osalStream osal_queue_open(
    const os_char *parameters,
	void *option,
	osalStatus *status,
	os_short flags)
{
    osalQueue *queue;
    os_memsz sz, true_sz;

	/* Parse queue buffer size from paramete string, for example "buf=1024". Default 
	   queue size is 512 bytes.
	 */
	sz = (os_memsz)osal_string_get_item_int(parameters, "buf", 512, 
		OSAL_STRING_SEARCH_LINE_ONLY);
	 
	/* Allocate queue structure.
	 */
	queue = os_malloc(sizeof(osalQueue), OS_NULL);
	if (queue == OS_NULL) goto memalloc_failed;

	/* Get value of "ctrl" flag. This determines if the queue is to use control codes.
	   If control codes are to be used, reserve extra space.
	 */
	queue->ctrl_support = (os_boolean)osal_string_get_item_int(parameters, "ctrl", 0, 
		OSAL_STRING_SEARCH_LINE_ONLY);
	if (queue->ctrl_support) sz += OSAL_QUEUE_CTRL_RESERVE;

	/* Allocate buffer for queue and save size in bytes.
	 */
	queue->qbuf = os_malloc(sz+1, &true_sz);
	if (queue->qbuf == OS_NULL)
	{
		os_free(queue, sizeof(osalQueue));
		goto memalloc_failed;
	}
	queue->sz = (os_memsz)true_sz;

	/* Reset the head and tail.
	 */
	queue->head = queue->tail = 0;

	/* Save open flags.
	 */
	queue->open_flags = flags;

	/* Save interface pointer.
	 */
#if OSAL_FUNCTION_POINTER_SUPPORT
	queue->hdr.iface = &osal_queue_iface;
#endif

	/* Set infinite timeout
	 */
	queue->hdr.write_timeout_ms = queue->hdr.read_timeout_ms = -1;

	/* Success set status code and cast queue structure pointer to stream pointer and return it.
	 */
	if (status) *status = OSAL_SUCCESS;
	return (osalStream)queue;

memalloc_failed:
	/* Memory allocation failed. Set status code and return null pointer.
	 */
	if (status) *status = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
	return OS_NULL;
}


/**
****************************************************************************************************

  @brief Delete the byte queue.
  @anchor osal_queue_close

  The osal_queue_close() function deletes a byte queue, which was creted by osal_queue_open() 
  function. All resource related to the queue are freed. Any attemp to use the queue after
  this call may result crash.

  @param   stream Stream pointer representing the queue. After this call stream pointer will
		   point to invalid memory location.
  @return  None.

****************************************************************************************************
*/
void osal_queue_close(
	osalStream stream)
{
    osalQueue *queue;

	if (stream)
	{
		queue = (osalQueue*)stream;

		/* Free queue buffer.
		 */
		os_free(queue->qbuf, queue->sz);

		/* Free queue.
		 */
		os_free(queue, sizeof(osalQueue));
	}
}


/**
****************************************************************************************************

  @brief Flush the byte queue.
  @anchor osal_queue_flush

  The osal_queue_flush() function flushes data to be written to stream.

  @param   stream Stream pointer representing the queue.
  @param   flags See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_queue_flush(
	osalStream stream,
	os_short flags)
{
	return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Write data to byte queue.
  @anchor osal_queue_write

  The osal_queue_write() function writes up to n bytes of data from buffer to queue.

  @param   stream Stream pointer representing the queue.
  @param   buf Pointer to the beginning of data to place into the queue.
  @param   n Maximum number of bytes to write. 
  @param   n_written Pointer to integer into which the function stores the number of bytes 
		   actually written to queue,  which may be less than n if there is not enough space
		   left in the queue. If the function fails n_written is set to zero.
  @param   flags Flags for the function.
		   See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_queue_write(
	osalStream stream,
	const os_uchar *buf,
	os_memsz n,
	os_memsz *n_written,
	os_short flags)
{
    osalQueue *queue;
    os_uchar *qbuf, prev_c;
    const os_uchar *d;
    os_memsz cnt, sz, head, tail, space, i, qu_n;
    os_boolean ctrl_support;

	if (stream)
	{
		queue = (osalQueue*)stream;

		#if OSAL_MULTITHREAD_SUPPORT
		if (queue->open_flags & OSAL_STREAM_SYNCHRONIZE)
		{
			os_lock();
		}
		#endif

		qbuf = queue->qbuf;
		sz = queue->sz;
		head = queue->head;
		tail = queue->tail;
		ctrl_support = queue->ctrl_support;
		d = buf;

		space = tail - head - 1;
		if (space < 0) space += sz;
		
		if (ctrl_support) 
		{
			space -= OSAL_QUEUE_CTRL_RESERVE;
			qu_n = 0;
			for (i = 0; i<n; ++i) 
			{
				cnt = (buf[i] == OSAL_QUEUE_CTRL_CODE) ? 2 : 1;
				qu_n += cnt;
				if (qu_n > space)
				{
					if (flags & OSAL_STREAM_ALL_OR_NOTHING)
					{
						*n_written = 0;
						goto getout;
					}
					n = i;
					qu_n -= cnt;
					break;
				}
			}
		}
		else
		{
			if (n > space) 
			{
				if (flags & OSAL_STREAM_ALL_OR_NOTHING)
				{
					*n_written = 0;
					goto getout;
				}

				n = space;
			}
			qu_n = n;
		}

		if (n <= 0)
		{
			*n_written = 0;
			goto getout;
		}
		*n_written = n;

		prev_c = 0;
		if (head >= tail)
		{
			cnt = sz - head;
			if (cnt > qu_n) cnt = qu_n;
			qu_n -= cnt;

			if (ctrl_support) 
			{
				while (cnt-- > 0)
				{
					if (prev_c == OSAL_QUEUE_CTRL_CODE) 
					{
						qbuf[head++] = (os_uchar)OSAL_STREAM_CTRL_CHAR;
						prev_c = 0;
					}
					else
					{
						prev_c = *(d++);
						qbuf[head++] = prev_c;
					}
				}
			}
			else
			{
				os_memcpy(qbuf + head, d, cnt);
				head += cnt;
				d += cnt;
			}

			if (head < sz)
			{
				queue->head = head;
				goto getout;
			}

			head = 0;
		}

		cnt = tail - head - 1;
		if (cnt > qu_n) cnt = qu_n;

		if (ctrl_support) 
		{
			while (cnt-- > 0)
			{
				if (prev_c == OSAL_QUEUE_CTRL_CODE) 
				{
					qbuf[head++] = (os_uchar)OSAL_STREAM_CTRL_CHAR;
					prev_c = 0;
				}
				else
				{
					prev_c = *(d++);
					qbuf[head++] = prev_c;
				}
			}
			queue->head = head;
		}
		else
		{
			os_memcpy(qbuf + head, d, cnt);
			queue->head = head + cnt;
		}

getout:
		#if OSAL_MULTITHREAD_SUPPORT
		if (queue->open_flags & OSAL_STREAM_SYNCHRONIZE)
		{
			os_unlock();
		}
		#endif

		return OSAL_SUCCESS;
	}

	*n_written = 0;
	return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Read data from byte queue.
  @anchor osal_queue_read

  The osal_queue_read() function reads up to n bytes of data from queue into buffer. 

  @param   stream Stream pointer representing the queue.
  @param   buf Pointer to buffer to read into.
  @param   n Maximum number of bytes to read. The data buffer must large enough to hold
		   at least this many bytes. 
  @param   n_read Pointer to integer into which the function stores the number of bytes read, 
           which may be less than n if there are fewer bytes available. If the function fails 
		   n_read is set to zero.
  @param   flags Flags for the function, use OSAL_STREAM_DEFAULT (0) for default operation. 
		   The OSAL_STREAM_PEEK flag causes the function to return data in queue, but nothing
		   will be removed from the queue.
		   See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_queue_read(
	osalStream stream,
	os_uchar *buf,
	os_memsz n,
	os_memsz *n_read,
	os_short flags)
{
    osalQueue *queue;
    os_uchar *qbuf, *d, prev_c;
    os_memsz cnt, sz, head, tail;

	if (stream)
	{
		queue = (osalQueue*)stream;

		#if OSAL_MULTITHREAD_SUPPORT
		if (queue->open_flags & OSAL_STREAM_SYNCHRONIZE)
		{
			os_lock();
		}
		#endif

		qbuf = queue->qbuf;
		sz = queue->sz;
		head = queue->head;
		tail = queue->tail;
		d = buf;

		if (queue->ctrl_support) 
		{
			cnt = 0;
			prev_c = 0;

			while (tail != head && cnt < n)
			{
				if (prev_c == OSAL_QUEUE_CTRL_CODE)
				{
					if (qbuf[tail++] == (OSAL_STREAM_CTRL_CHAR&0xFF)) 
						d[cnt++] = OSAL_QUEUE_CTRL_CODE;
					prev_c = 0;
				}
				else
				{
					prev_c = qbuf[tail++];
					if (prev_c != OSAL_QUEUE_CTRL_CODE) d[cnt++] = prev_c;
				}
				if (tail >= sz) tail = 0;
			}

			if ((flags & OSAL_STREAM_PEEK) == 0) queue->tail = tail;
			*n_read = cnt;
		}
		else
		{
			if (tail > head)
			{
				cnt = sz - tail;
				if (cnt > n) cnt = n;

				os_memcpy(d, qbuf + tail, cnt);

				tail += cnt;
				if (tail < sz)
				{
					if ((flags & OSAL_STREAM_PEEK) == 0) queue->tail = tail;
					*n_read = cnt;
					goto getout;
				}
				tail = 0;
				n -= cnt;
				d += cnt;
			}

			cnt = head - tail;
			if (cnt > n) cnt = n;

			if (cnt > 0) os_memcpy(d, qbuf + tail, cnt);

			if ((flags & OSAL_STREAM_PEEK) == 0) queue->tail = tail + cnt;
			*n_read = d - buf + cnt;
		}

getout:
		#if OSAL_MULTITHREAD_SUPPORT
		if (queue->open_flags & OSAL_STREAM_SYNCHRONIZE)
		{
			os_unlock();
		}
		#endif

		return OSAL_SUCCESS;
	}

	*n_read = 0;
	return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Write a value to byte queue.
  @anchor osal_queue_write_value

  The osal_queue_write_value() function places a value into queue.

  @param   stream Stream pointer representing the queue.
  @param   c Byte value to write into queue.
  @param   flags Flags for the function.
		   - The OSAL_STREAM_NO_REPEATED_CTRLS flag causes the function to check if the same control 
		   code is already last item of the queue. If so, tre repeated control code is not written.

		   See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success. 
		   Value OSAL_STATUS_STREAM_WOULD_BLOCK indicates that character could not be 
		   written because the buffer was full. All other nonzero values indicate an error. 
		   See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_queue_write_value(
	osalStream stream,
	os_ushort c,
	os_short flags)
{
    osalQueue *queue;
    os_memsz head, next_head, prev_head, tail, sz, space;
    osalStatus rval;
    os_boolean ctrl_support;

	if (stream)
	{
		queue = (osalQueue*)stream;

		#if OSAL_MULTITHREAD_SUPPORT
		if (queue->open_flags & OSAL_STREAM_SYNCHRONIZE)
		{
			os_lock();
		}
		#endif

		head = queue->head;
		tail = queue->tail;
		sz = queue->sz;
		ctrl_support = queue->ctrl_support;

		/* If contol code support is used.
		 */
		if (ctrl_support)
		{
			/* If c is usual byte value, do not write to space in queue 
			   reserved for control codes.
			 */
			if (c < 256)
			{
				space = tail - head - 1;
				if (space < 0) space += sz;
				if (space <= OSAL_QUEUE_CTRL_RESERVE) goto getout;
			}

			/* If the control codes are not to be repeated, check if this
			   is repeate of the previous control code in queue.
			 */
			else if ((flags & OSAL_STREAM_NO_REPEATED_CTRLS) && head != tail)
			{
				prev_head = head-1;
				if (prev_head < 0) prev_head = sz-1;
				if (queue->qbuf[prev_head] == (c & 0xFF) &&
					prev_head != tail)
				{
					if (--prev_head < 0) prev_head = sz-1;
					if (queue->qbuf[prev_head] == OSAL_QUEUE_CTRL_CODE)
					{
						rval = OSAL_SUCCESS;
						goto getout2;
					}
				}
			}
		}

		next_head = head + 1;

		if (next_head >= sz) next_head = 0;
		if (next_head != tail)
		{
			if (ctrl_support)
			{
				if (c >= 256 || c == OSAL_QUEUE_CTRL_CODE)
				{
					queue->qbuf[head] = OSAL_QUEUE_CTRL_CODE;
					head = next_head;

					if (++next_head >= sz) next_head = 0;
					if (next_head == tail) goto getout;

					if (c == OSAL_QUEUE_CTRL_CODE) c = OSAL_STREAM_CTRL_CHAR;
				}
			}

			queue->qbuf[head] = (os_uchar)c;
			queue->head = next_head;
			rval = OSAL_SUCCESS;
		}
		else
		{
getout:
			rval = OSAL_STATUS_STREAM_WOULD_BLOCK;
		}

getout2:
		#if OSAL_MULTITHREAD_SUPPORT
		if (queue->open_flags & OSAL_STREAM_SYNCHRONIZE)
		{
			os_unlock();
		}
		#endif
		
		return rval;
	}

	return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Read a value from byte queue.
  @anchor osal_queue_read_value

  The osal_queue_read_value() function reads a value from queue.

  @param   stream Stream pointer representing the queue.
  @param   c Pointer to integer into which to store byte read from queue.
  @param   flags Flags for the function, use OSAL_STREAM_DEFAULT (0) for default operation. 
		   The OSAL_STREAM_PEEK flag causes the function to return data in queue, but nothing
		   will be removed from the queue.
		   See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success. Value 
		   OSAL_STATUS_STREAM_WOULD_BLOCK indicates that there was no character to be read.
		   All other nonzero values indicate an error. 
		   See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_queue_read_value(
	osalStream stream,
	os_ushort *c,
	os_short flags)
{
    osalQueue *queue;
    os_memsz head, tail, sz;
    osalStatus rval;
    os_ushort cc, nc;

	if (stream)
	{
		queue = (osalQueue*)stream;

		#if OSAL_MULTITHREAD_SUPPORT
		if (queue->open_flags & OSAL_STREAM_SYNCHRONIZE)
		{
			os_lock();
		}
		#endif

		tail = queue->tail;
		head = queue->head;
		sz = queue->sz;

		if (tail != head)
		{
			cc = queue->qbuf[tail];

			if (++tail >= sz) tail = 0;

			if (queue->ctrl_support)
			{
				if (cc == OSAL_QUEUE_CTRL_CODE) 
				{
					if (tail == head) goto getout;
					nc = queue->qbuf[tail];
					if (nc != (OSAL_STREAM_CTRL_CHAR&0xFF)) cc = nc + 256;
					if (++tail >= sz) tail = 0;
				}
			}

			*c = cc;
			if ((flags & OSAL_STREAM_PEEK) == 0) queue->tail = tail;
			rval = OSAL_SUCCESS;
		}
		else
		{
getout:
			rval = OSAL_STATUS_STREAM_WOULD_BLOCK;
			*c = 0;
		}

		#if OSAL_MULTITHREAD_SUPPORT
		if (queue->open_flags & OSAL_STREAM_SYNCHRONIZE)
		{
			os_unlock();
		}
		#endif

		return rval;
	}

	*c = 0;
	return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Get byte queue parameter.
  @anchor osal_queue_get_parameter

  The osal_queue_get_parameter() function gets a parameter value.

  @param   stream Stream pointer representing the queue.
  @param   parameter_ix Index of parameter to get.
		   See @ref osalStreamParameterIx "stream parameter enumeration" for the list.
  @return  Parameter value.

****************************************************************************************************
*/
os_long osal_queue_get_parameter(
	osalStream stream,
	osalStreamParameterIx parameter_ix)
{
	/* Call the default implementation
	 */
	return osal_stream_default_get_parameter(stream, parameter_ix);
}


/**
****************************************************************************************************

  @brief Set byte queue parameter.
  @anchor osal_queue_set_parameter

  The osal_queue_set_parameter() function gets a parameter value.

  @param   stream Stream pointer representing the queue.
  @param   parameter_ix Index of parameter to get.
		   See @ref osalStreamParameterIx "stream parameter enumeration" for the list.
  @param   value Parameter value to set.
  @return  None.

****************************************************************************************************
*/
void osal_queue_set_parameter(
	osalStream stream,
	osalStreamParameterIx parameter_ix,
	os_long value)
{
	/* Call the default implementation
	 */
	osal_stream_default_set_parameter(stream, parameter_ix, value);
}


#if OSAL_FUNCTION_POINTER_SUPPORT

const osalStreamInterface osal_queue_iface
 = {osal_queue_open,
	osal_queue_close,
	osal_stream_default_accept,
	osal_queue_flush,
	osal_stream_default_seek,
	osal_queue_write,
	osal_queue_read,
	osal_queue_write_value,
	osal_queue_read_value,
	osal_queue_get_parameter,
	osal_queue_set_parameter};

#endif

#endif
