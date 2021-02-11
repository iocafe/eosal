/**

  @file    eosal_jpeg/eosal_code/osal_jpeg.h
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

/* Flags for os_compress_JPEG() and os_uncompress_JPEG() functions.
 */
#define OSAL_JPEG_DEFAULT 0
#define OSAL_JPEG_SELECT_ALPHA_CHANNEL 1

/* Structure for managing uncomression bitmap.
 */
typedef struct osalJpegMallocContext
{
    os_uchar *buf;
    os_memsz buf_sz;
    os_memsz nbytes;
    os_int w, h;
    osalBitmapFormat format;
}
osalJpegMallocContext;

/* Function to customize memory allocation for uncompressed bitmap.
 */
typedef osalStatus osal_jpeg_malloc_func(
    osalJpegMallocContext *context,
    os_memsz nbytes);

/* Convert a bitmap in memory to JPEG.
 */
osalStatus os_compress_JPEG(
    os_uchar *src,
    os_int w,
    os_int h,
    os_int row_nbytes,
    osalBitmapFormat format,
    os_int quality,
    osalStream dst_stream,
    os_uchar *dst_buf,
    os_memsz dst_buf_sz,
    os_memsz *dst_nbytes,
    os_int flags);

/* Uncompress JPEG to bitmap in memory.
 */
osalStatus os_uncompress_JPEG(
    os_uchar *src_buf,
    os_memsz src_nbytes,
    osal_jpeg_malloc_func *alloc_func,
    osalJpegMallocContext *alloc_context,
    os_int flags);
