/**

  @file    eosal_jpeg/eosal_code/osal_jpeg_src.c
  @brief   eosal API for libjpeg.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    5.5.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

  Copyright (C) 1991-1998, Thomas G. Lane. This file is derived from work of the Independent
  JPEG Group's software.

****************************************************************************************************
*/
#include "eosal_jpeg.h"
#include "eosal_code/osal_jpeg_src.h"

/** Source manager structure for uncompressing JPEGs. This must start with jpeg_source_mgr
    which sets up public fields needed by libjpeg.
 */
typedef struct osaJpegSrcManager
{
    struct jpeg_source_mgr pub;
}
osaJpegSrcManager;


/**
****************************************************************************************************

  @brief Callback function for the JPEG lib, initialize source.

  The osal_jpeg_init_source function is helper callback, which should
  initialize the data source. This function is
  called by jpeg_read_header before any data is actually read.

  @param  dp JPEG library's internal state structure pointer?
  @return None.

****************************************************************************************************
*/
static void osal_jpeg_init_source(
    j_decompress_ptr dp)
{
}


/**
****************************************************************************************************

  @brief Callback function for the JPEG lib, read more (called only on
         error).

  The osal_jpeg_fill_input_buffer callback, which by should fill the input buffer, is called
  whenever JPEG decompression runs out of data.

  @param  dp JPEG library's internal state structure pointer?
  @return Always TRUE.

****************************************************************************************************
*/
static int osal_jpeg_fill_input_buffer(
    j_decompress_ptr dp)
{
    ERREXIT(dp, JERR_INPUT_EMPTY);
    return TRUE;
}


/**
****************************************************************************************************

  @brief Callback function for the JPEG lib, skip some input data.

  The osal_jpeg_skip_input_data callback skips input data. This function is used to skip over
  a potentially large amount of uninteresting data (such as an APPn marker).

  @param  dp JPEG library's internal state structure pointer?
  @return None.

****************************************************************************************************
*/
static void osal_jpeg_skip_input_data(
    j_decompress_ptr dp,
    long num_bytes)
{
    osaJpegSrcManager * src = (osaJpegSrcManager *) dp->src;

    if (num_bytes > 0)
    {
        src->pub.next_input_byte += (size_t) num_bytes;
        src->pub.bytes_in_buffer -= (size_t) num_bytes;
    }
}


/**
****************************************************************************************************

  @brief Callback function for the JPEG lib, skip some input data.

  The osal_jpeg_term_source function is helper callback, which should terminate
  the data source.

  NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
  application must deal with any cleanup that should happen even
  for error exit.

  @param  dp JPEG library's internal state structure pointer?
  @return None.

****************************************************************************************************
*/
static void osal_jpeg_term_source(
    j_decompress_ptr dp)
{
}


/**
****************************************************************************************************

  @brief Prepare for JPEG uncompression.

  The osal_jpeg_setup_source function prepares j_decompress structure cprm for JPEG input.

  @param  cprm Pointer JPEG library decompression structure to set up.
  @param  src_steram

  @return None.

****************************************************************************************************
*/
void osal_jpeg_setup_source(
    void *cprm,
    os_uchar *src_buf,
    os_memsz src_nbytes)
{
    osaJpegSrcManager * src;
    j_decompress_ptr dp = cprm;

    if (dp->src == NULL)
    {
        dp->src = (struct jpeg_source_mgr *)
          (*dp->mem->alloc_small) ((j_common_ptr) dp, JPOOL_PERMANENT,
                  sizeof(osaJpegSrcManager));

        os_memclear(dp->src, sizeof(osaJpegSrcManager));
    }

    src = (osaJpegSrcManager *) dp->src;
    src->pub.init_source = osal_jpeg_init_source;
    src->pub.fill_input_buffer = osal_jpeg_fill_input_buffer;
    src->pub.skip_input_data = osal_jpeg_skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->pub.term_source = osal_jpeg_term_source;
    src->pub.bytes_in_buffer = src_nbytes;
    src->pub.next_input_byte = (os_char*)src_buf;
}
