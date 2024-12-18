/**

  @file    stream/common/osal_stream_buffer.c
  @brief   Memory buffer with OSAL stream API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Memory buffer, which implements OSAL stream interface. This class presents simple buffer
  in memory as a stream for reading and writing.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_STREAM_BUFFER_SUPPORT

/** Stream buffer data structure.
 */
typedef struct osalStreamBuffer
{
    /** A stream structure must start with this generic stream header structure.
     */
    osalStreamHeader hdr;

    /** Pointer to allocated buffer.
     */
    os_char *ptr;

    /** Allocated buffer size in bytes.
     */
    os_memsz sz;

    /** Used buffer size for write (current write position).
     */
    os_memsz n;

    /** Current read position.
     */
    os_memsz read_pos;
}
osalStreamBuffer;


/**
****************************************************************************************************

  @brief Open a stream buffer.
  @anchor osal_stream_buffer_open

  The osal_stream_buffer_open() function opens a stream_buffer.

  @param  parameters Ignored, set OS_NULL or empty string.
  @param  option Not used for stream_buffers, set OS_NULL.
  @param  status Pointer to integer into which to store the function status code. Value
          OSAL_SUCCESS (0) indicates success and OSAL_STATUS_MEMORY_ALLOCATION_FAILED
          a memory allocation problem.
  @param   flags Ignored, set OSAL_STREAM_DEFAULT (0).

  @return Stream pointer representing the new stream buffer.

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
    OSAL_UNUSED(parameters);
    OSAL_UNUSED(option);
    OSAL_UNUSED(flags);

    sbuf = (osalStreamBuffer*)os_malloc(sizeof(osalStreamBuffer), OS_NULL);
    if (sbuf == OS_NULL)
    {
        s = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        goto getout;
    }
    os_memclear(sbuf, sizeof(osalStreamBuffer));
    sbuf->hdr.iface = &osal_stream_buffer_iface;

getout:
    if (status) *status = s;
    return (osalStream)sbuf;
}


/**
****************************************************************************************************

  @brief Close stream buffer.
  @anchor osal_stream_buffer_close

  The osal_socket_close() function closes the stream buffer and releases memory allocated for it.
  Any attempt to use the stream buffer after this call may result crash.

  @param   stream Pointer to the stream buffer object.
  @return  None.

****************************************************************************************************
*/
void osal_stream_buffer_close(
    osalStream stream,
    os_int flags)
{
    osalStreamBuffer *sbuf;
    OSAL_UNUSED(flags);

    if (stream == OS_NULL) return;
    sbuf = (osalStreamBuffer*)stream;

    if (sbuf->sz) {
        os_free(sbuf->ptr, sbuf->sz);
    }

    os_free(sbuf, sizeof(osalStreamBuffer));
}


/**
****************************************************************************************************

  @brief Get or set current read or write position.
  @anchor osal_stream_buffer_seek

  The osal_stream_buffer_seek() function gets or optionally sets current read or write position.

  @param   stream Pointer to the stream buffer object.
  @param   pos Pointer to integer which contains read/write position to get/set.
  @param   flags OSAL_STREAM_SEEK_WRITE_POS to select write position, otherwise read position
           is selected.
           OSAL_STREAM_SEEK_SET to set seek position, without this flag the function only
           returns the position.

  @return  Function status code. OSAL_SUCCESS (0) indicates success and value
           OSAL_STATUS_MEMORY_ALLOCATION_FAILED failed memory allocation.

****************************************************************************************************
*/
osalStatus osal_stream_buffer_seek(
    osalStream stream,
    os_long *pos,
    os_int flags)
{
    osalStreamBuffer *sbuf;

    if (stream == OS_NULL) return OSAL_STATUS_FAILED;
    sbuf = (osalStreamBuffer*)stream;

    if (flags & OSAL_STREAM_SEEK_WRITE_POS)
    {
        if (flags & OSAL_STREAM_SEEK_SET)
        {
            sbuf->n = *pos;
        }

        *pos = sbuf->n;
    }
    else
    {
        if (flags & OSAL_STREAM_SEEK_SET)
        {
            sbuf->read_pos = *pos;
        }

        *pos = sbuf->read_pos;
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Write data to stream buffer.
  @anchor osal_stream_buffer_write

  The osal_stream_buffer_write() function writes up to n bytes of data to stream_buffer.

  @param   stream Pointer to the stream buffer object.
  @param   buf Pointer to the beginning of data to place into the stream_buffer.
  @param   n Maximum number of bytes to write.
  @param   n_written Always set to n, unless memory allocation fails (set to 0).
  @param   flags Ignored, set OSAL_STREAM_DEFAULT (0).
  @return  Function status code. OSAL_SUCCESS (0) indicates success and value
           OSAL_STATUS_MEMORY_ALLOCATION_FAILED failed memory allocation.

****************************************************************************************************
*/
osalStatus osal_stream_buffer_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags)
{
    osalStreamBuffer *sbuf;
    os_memsz try_sz;
    osalStatus s;
    OSAL_UNUSED(flags);

    sbuf = (osalStreamBuffer*)stream;
    if (sbuf->n + n > sbuf->sz)
    {
        try_sz = 5*sbuf->sz/3 + n;
        if (try_sz < 64) try_sz = 64;

        s = osal_stream_buffer_realloc(stream, try_sz);
        if (s) {
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

  @brief Read data from stream buffer.
  @anchor osal_stream_buffer_read

  The osal_stream_buffer_read() function reads up to n bytes of data from stream buffer.

  @param   stream Pointer to the stream buffer object.
  @param   buf Pointer to buffer to read into.
  @param   n Maximum number of bytes to read. The data buffer must large enough to hold
           at least this many bytes.
  @param   n_read Pointer to integer into which the function stores the number of bytes read,
           which may be less than n if there are fewer bytes available. If the function fails
           n_read is set to zero.
  @param   flags Ignored, set OSAL_STREAM_DEFAULT (0).

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error.

****************************************************************************************************
*/
osalStatus osal_stream_buffer_read(
    osalStream stream,
    os_char *buf,
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

  @param   stream Pointer to the stream buffer object.
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

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Get pointer to buffer content.
  @anchor osal_stream_buffer_content

  The osal_stream_buffer_content() function returns pointer to stream buffer content and sets
  n to content size in bytes.

  @param   stream Pointer to the stream buffer object.
  @param   n Pointer to integer where to store content size in bytes. Can be OS_NULL if not needed.

  @return  Pointer to stream buffer content.

****************************************************************************************************
*/
os_char *osal_stream_buffer_content(
    osalStream stream,
    os_memsz *n)
{
    osalStreamBuffer *sbuf;
    sbuf = (osalStreamBuffer*)stream;

    if (n) *n = sbuf->n;
    return sbuf->ptr;
}


/**
****************************************************************************************************

  @brief Get stream buffer content and take ownership of it.
  @anchor osal_stream_buffer_adopt_content

  The osal_stream_buffer_adopt_content() function returns pointer to stream buffer content and sets
  n to content size and n_alloc to memory allocation for content.

  THIS FUNCTTION MOVES OWNERSHIP OF CONTENT BUFFER FROM STREAM TO CALLER. Caller is responsible
  for releasing content buffer by os_free(content, n) or os_free(content, alloc_n). The stream
  is empty after this call. New writes to it will start a new buffer.

  @param   stream Pointer to the stream buffer object.
  @param   n Pointer to integer where to store content size in bytes. Can be OS_NULL if not needed.
  @param   alloc_n Pointer to integer where to store buffer allocation size in bytes.
           Can be OS_NULL if not needed.

  @return  Pointer to stream buffer content.

****************************************************************************************************
*/
os_char *osal_stream_buffer_adopt_content(
    osalStream stream,
    os_memsz *n,
    os_memsz *alloc_n)
{
    os_char *ptr;
    osalStreamBuffer *sbuf;
    sbuf = (osalStreamBuffer*)stream;

    if (n) *n = sbuf->n;
    if (alloc_n) *alloc_n = sbuf->sz;
    ptr = sbuf->ptr;

    sbuf->ptr = OS_NULL;
    sbuf->sz = sbuf->n = 0;
    sbuf->read_pos = 0;

    return ptr;
}


/** Stream interface for OSAL stream_buffers. This is structure osalStreamInterface filled with
    function pointers to OSAL stream_buffers implementation.
 */
const osalStreamInterface osal_stream_buffer_iface
 = {OSAL_STREAM_IFLAG_NONE,
    osal_stream_buffer_open,
    osal_stream_buffer_close,
    osal_stream_default_accept,
    osal_stream_default_flush,
    osal_stream_buffer_seek,
    osal_stream_buffer_write,
    osal_stream_buffer_read,
    osal_stream_default_select,
    OS_NULL,
    OS_NULL
    };

#endif

