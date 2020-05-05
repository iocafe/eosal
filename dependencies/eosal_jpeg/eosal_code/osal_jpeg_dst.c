/**

  @file    eosal_jpeg/eosal_code/osal_jpeg_dst.c
  @brief   eosal API for libjpeg.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    5.5.2020

  This file contains simple error-reporting and trace-message routines. These routines are
  used by both the compression and decompression code.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

  Copyright (C) 1991-1998, Thomas G. Lane. This file is derived from work of the Independent
  JPEG Group's software.

****************************************************************************************************
*/
#include "eosal_jpeg.h"
#include "eosal_code/osal_jpeg_dst.h"

/** JPEG destination manager. This must start with jpeg_destination_mgr which sets up public
    fields needed by libjpeg.
 */
typedef struct
{
    /* Public fields, defined by libjpeg
     */
    struct jpeg_destination_mgr pub;

    osalStream dst_stream;
    os_uchar *dst_buf;
    os_memsz dst_buf_sz;
    os_memsz dst_nbytes;
    osalStatus status;
    os_boolean buf_allocated;
}
osaJpegDstManager;


/**
****************************************************************************************************

  @brief Callback function for the JPEG lib, initialize destination.

  The osal_jpeg_init_destination function initializes destination for writing. This function is
  called by jpeg_start_compress before any data is actually written.

  @param  cinfo JPEG library's state structure
  @return None.

****************************************************************************************************
*/
static void osal_jpeg_init_destination(
    j_compress_ptr cinfo)
{
    osaJpegDstManager *dest;
    dest = (osaJpegDstManager *) cinfo->dest;

    dest->pub.next_output_byte = dest->dst_buf;
    dest->pub.free_in_buffer = (size_t)dest->dst_buf_sz;
}


/**
****************************************************************************************************

  @brief Callback function for the JPEG lib, flush JPEG output buffer.

  The osal_jpeg_empty_output_buffer callback empties the JPEG output buffer -- called whenever
  the buffer fills up.

  In typical applications, this should write the entire output buffer (ignoring the current state
  of next_output_byte & free_in_buffer), reset the pointer & count to the start of the buffer, and
  return TRUE indicating that the buffer has been dumped.

  @param  cinfo JPEG library's internal state structure pointer?
  @return Always osal_jpeg_TRUE to indicate success, and that there is no
          need to stop the JPEG output.

****************************************************************************************************
*/
static boolean osal_jpeg_empty_output_buffer(
    j_compress_ptr cinfo)
{
    os_memsz n_written, n;
    osalStatus s;
    osaJpegDstManager *dest;
    dest = (osaJpegDstManager*)cinfo->dest;

    n = (os_uchar*)dest->pub.next_output_byte - dest->dst_buf;

    if (dest->dst_stream)
    {
        s = osal_stream_write(dest->dst_stream, (os_char*)dest->dst_buf,
            n, &n_written, OSAL_STREAM_DEFAULT);
        if (s || n_written != n) {
            dest->status = OSAL_STATUS_FAILED;
        }
    }
    else {
        dest->status = OSAL_STATUS_FAILED;
    }

    dest->dst_nbytes += n;
    dest->pub.next_output_byte = dest->dst_buf;
    dest->pub.free_in_buffer = (size_t)dest->dst_buf_sz;

    return TRUE;
}


/**
****************************************************************************************************

  @brief Callback function for the JPEG lib, terminate output buffer.

  The osal_jpeg_term_destination function is helper callback, which terminates the destination
  --- called by jpeg_finish_compress after all data has  been written. Usually this function
  would need to flush buffer.

  NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
  application must deal with any cleanup that should happen even
  for error exit.

  @param  cinfo JPEG library's internal state structure pointer?
  @return None.

****************************************************************************************************
*/
METHODDEF(void) osal_jpeg_term_destination(
    j_compress_ptr cinfo)
{
    os_memsz n, n_written;
    osalStatus s;
    osaJpegDstManager *dest;
    dest = (osaJpegDstManager*)cinfo->dest;

    n = (os_uchar*)dest->pub.next_output_byte - dest->dst_buf;

    if (dest->dst_stream)
    {
        s = osal_stream_write(dest->dst_stream, (os_char*)dest->dst_buf, n,
            &n_written, OSAL_STREAM_DEFAULT);
        if (s || n_written != n) {
            dest->status = OSAL_STATUS_FAILED;
        }
    }

    dest->dst_nbytes += n;
}


/**
****************************************************************************************************

  @brief Prepare for JPEG compression destination.

  The osal_jpeg_setup_destination function prepares jpeg_compress structure cprm for JPEG output.

  @param   cprm Pointer JPEG library compression structure to set up.
  @param   dst_stream Stream where to store the resulting JPEG. OS_NULL if storing JPEG to
           application allocated buffer dst_buf.
  @param   dst_buf Pointer to buffer where to store resulting JPEG or to use as intermediate
           buffer when saving to stream. If OS_NULL, then buffer is allocated.
  @param   dst_buf_sz Destination buffer size.
  @return  OSAL_SUCCESS if all is fine. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus osal_jpeg_setup_destination(
    void *cprm,
    osalStream dst_stream,
    os_uchar *dst_buf,
    os_memsz dst_buf_sz)
{
    osaJpegDstManager * dest;
    j_compress_ptr cinfo;

    cinfo = (j_compress_ptr)cprm;
    if (cinfo->dest == NULL)
    {
        cinfo->dest = (struct jpeg_destination_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo,
            JPOOL_PERMANENT, SIZEOF(osaJpegDstManager));

        if (cinfo->dest == OS_NULL) {
            return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        }
        os_memclear(cinfo->dest, SIZEOF(osaJpegDstManager));
    }

    dest = (osaJpegDstManager *) cinfo->dest;
    dest->pub.init_destination = osal_jpeg_init_destination;
    dest->pub.empty_output_buffer = osal_jpeg_empty_output_buffer;
    dest->pub.term_destination = osal_jpeg_term_destination;

    dest->dst_stream = dst_stream;

    if (dst_buf)
    {
        dest->dst_buf = dst_buf;
        dest->dst_buf_sz = dst_buf_sz;
        dest->buf_allocated = OS_FALSE;
    }
    else
    {
        dest->dst_buf = (os_uchar*)os_malloc(0x8000, &dest->dst_buf_sz);
        dest->buf_allocated = OS_TRUE;
    }
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Release memory, get number of JPEG bytes and status code.

  The osal_jpeg_destination_finished function gets number of bytes in reulting JPEG and
  status code associated with compression. The function also releases buffer if it
  was allocated by osal_jpeg_setup_destination.

  @param   cprm Pointer JPEG library compression structure to set up.
  @param   dst_nbytes Pointer where to store resulting JPEG size upon successfull compression.
  @return  OSAL_SUCCESS if all is fine. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus osal_jpeg_destination_finished(
    void *cprm,
    os_memsz *dst_nbytes)
{
    osaJpegDstManager * dest;
    j_compress_ptr cinfo;
    osalStatus s;

    cinfo = (j_compress_ptr)cprm;
    dest = (osaJpegDstManager *) cinfo->dest;
    if (dest)
    {
        *dst_nbytes = dest->dst_nbytes;
        s = dest->status;

        if (dest->buf_allocated)
        {
            os_free(dest->dst_buf, dest->dst_buf_sz);
        }
    }
    else
    {
        *dst_nbytes = 0;
        s = OSAL_STATUS_FAILED;
    }

    return s;
}

