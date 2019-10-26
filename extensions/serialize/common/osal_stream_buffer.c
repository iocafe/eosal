/**

  @file    osal_stream_buffer.c
  @brief   OSAL stream_buffers.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    25.10.2019

  Implementation of OSAL stream_buffers.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_SERIALIZE_SUPPORT


/** Stream buffer data structure.
 */
typedef struct osalStreamBuffer
{
    /** A stream structure must start with this generic stream header structure, which contains
        parameters common to every stream.
     */
    osalStreamHeader hdr;

    /** Pointer to allocated buffer.
     */
    os_char *ptr;

    /** Allocated buffer size in bytes.
     */
    os_memsz sz;

    /** Used buffer size for write.
     */
    os_memsz n;
}
osalStreamBuffer;





/* void osal_stream_append_long(
    osalStreamBuffer *buf,
    os_long value)
{
    os_char tmp[OSAL_INTSER_BUF_SZ];
    os_int tmp_n;
    os_memsz n_written;

    tmp_n = osal_intser_writer(tmp, z);
    osal_parse_json_append_buf(buf, tmp, tmp_sz);

    return osal_stream_buffer_write(stream, tmp, tmp_n,
    os_memsz n,
    os_memsz *n_written,
    os_int flags)
}
*/



/**
****************************************************************************************************

  @brief Open a stream buffer.
  @anchor osal_stream_buffer_open

  The osal_stream_buffer_open() function opens a stream_buffer.

  @param  parameters Ignored, set OS_NULL or empty string.
  @param  option Not used for stream_buffers, set OS_NULL.
  @param  status Pointer to integer into which to store the function status code. Value
		  OSAL_SUCCESS (0) indicates success and all nonzero values indicate an error.
          See @ref osalStatus "OSAL function return codes" for full list.
		  This parameter can be OS_NULL, if no status code is needed. 

  @param  flags Ignored, set 0.

  @return Stream pointer representing the stream_buffer.

****************************************************************************************************
*/
osalStream osal_stream_buffer_open(
    const os_char *parameters,
	void *option,
	osalStatus *status,
	os_int flags)
{
    osalStreamBuffer *sbuf;
    osalStatus s;

    sbuf = (osalStreamBuffer*)os_malloc(sizeof(osalStreamBuffer), OS_NULL);
    if (sbuf == OS_NULL)
    {
        s = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        goto getout;
    }
    os_memclear(sbuf, sizeof(sbuf));

    s = osal_stream_buffer_realloc((osalStream)sbuf, 64);
    if (s)
    {
        os_free(sbuf, sizeof(osalStreamBuffer));
        goto getout;
    }

getout:
    if (status) *status = s;
    return (osalStream)sbuf;
}


/**
****************************************************************************************************

  @brief Close socket.
  @anchor osal_socket_close

  The osal_socket_close() function closes a socket, which was creted by osal_socket_open()
  function. All resource related to the socket are freed. Any attemp to use the socket after
  this call may result crash.

  @param   stream Stream pointer representing the socket. After this call stream pointer will
           point to invalid memory location.
  @return  None.

****************************************************************************************************
*/
void osal_stream_buffer_close(
    osalStream stream)
{
    osalStreamBuffer *sbuf;

    if (stream == OS_NULL) return;
    sbuf = (osalStreamBuffer*)stream;

    if (sbuf->sz)
    {
        os_free(sbuf->ptr, sbuf->sz);
    }

    sbuf->ptr = OS_NULL;
    sbuf->sz = sbuf->n = 0;
}



/**
****************************************************************************************************

  @brief Write data to stream_buffer.
  @anchor osal_stream_buffer_write

  The osal_stream_buffer_write() function writes up to n bytes of data from buffer to stream_buffer.

  @param   stream Stream pointer representing the stream_buffer.
  @param   buf Pointer to the beginning of data to place into the stream_buffer.
  @param   n Maximum number of bytes to write. 
  @param   n_written Pointer to integer into which the function stores the number of bytes 
           actually written to stream_buffer,  which may be less than n if there is not enough space
           left in the stream_buffer. If the function fails n_written is set to zero.
  @param   flags Flags for the function.
		   See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_stream_buffer_write(
	osalStream stream,
	const os_uchar *buf,
	os_memsz n,
	os_memsz *n_written,
	os_int flags)
{
    osalStreamBuffer *sbuf;
    osalStatus s;

    sbuf = (osalStreamBuffer*)stream;
    if (sbuf->n + n > sbuf->sz)
    {
        s = osal_stream_buffer_realloc(stream, 2*sbuf->sz + n);
        if (s)
        {
            *n_written = 0;
            return s;
        }
    }
    os_memcpy(sbuf->ptr + sbuf->n, buf, n);
    sbuf->n += n;
    *n_written = n;
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Read data from stream_buffer.
  @anchor osal_stream_buffer_read

  The osal_stream_buffer_read() function reads up to n bytes of data from stream_buffer into buffer.

  @param   stream Stream pointer representing the stream_buffer.
  @param   buf Pointer to buffer to read into.
  @param   n Maximum number of bytes to read. The data buffer must large enough to hold
           at least this many bytes.
  @param   n_read Pointer to integer into which the function stores the number of bytes read, 
           which may be less than n if there are fewer bytes available. If the function fails 
		   n_read is set to zero.
  @param   flags Flags for the function, use OSAL_STREAM_DEFAULT (0) for default operation. 
           The OSAL_STREAM_PEEK flag causes the function to return data in stream_buffer, but nothing
           will be removed from the stream_buffer.
		   See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_stream_buffer_read(
	osalStream stream,
	os_uchar *buf,
	os_memsz n,
	os_memsz *n_read,
	os_int flags)
{
    *n_read = 0;
    return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Allocate more memory for the buffer.
  @anchor osal_stream_buffer_realloc

  The osal_stream_buffer_realloc() function makes sure that buffer is at least request_sz bytes.
  Data in buffer is preserved (n bytes).

  @param   stream Stream pointer representing the stream buffer.
  @param   request_sz Minimum number of bytes required.

  @return  Function status code. OSAL_SUCCESS (0) indicates success and value
           OSAL_STATUS_MEMORY_ALLOCATION_FAILED failed memory allocation.

****************************************************************************************************
*/
osalStatus osal_stream_buffer_realloc(
    osalStream stream,
    os_memsz request_sz)
{
    osalStreamBuffer *sbuf;
    os_char *new_ptr;
    os_memsz new_sz, n;

    sbuf = (osalStreamBuffer*)stream;
    if (request_sz <= sbuf->sz) return OSAL_SUCCESS;

    new_ptr = os_malloc(request_sz, &new_sz);
    if (new_ptr == OS_NULL)
    {
        return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
    }

    n = sbuf->n;
    if (n)
    {
        os_memcpy(new_ptr, sbuf->ptr, n);
    }

    if (sbuf->sz)
    {
        os_free(sbuf->ptr, sbuf->sz);
    }

    sbuf->ptr = new_ptr;
    sbuf->sz = new_sz;
    sbuf->n = n;

    return OSAL_SUCCESS;
}


#if OSAL_FUNCTION_POINTER_SUPPORT

/** Stream interface for OSAL stream_buffers. This is structure osalStreamInterface filled with
    function pointers to OSAL stream_buffers implementation.
 */
const osalStreamInterface osal_stream_buffer_iface
 = {osal_stream_buffer_open,
    osal_stream_buffer_close,
    osal_stream_default_accept,
    osal_stream_default_flush,
	osal_stream_default_seek,
    osal_stream_buffer_write,
    osal_stream_buffer_read,
	osal_stream_default_write_value,
	osal_stream_default_read_value,
    osal_stream_default_get_parameter,
    osal_stream_default_set_parameter,
    osal_stream_default_select};
#endif

#endif

