/**

  @file    eosal_jpeg.h
  @brief   libjpeg
  @author  Pekka Lehtikoski
  @version 1.0
  @date    2.5.2020

  This is the original old libjpeg, jpegsr6.zip, dated 6b with minor changes
  to organization.

  - Memory allocation uses eosal (operating system abstraction layer) functions.
  - Global external names staring with "esoal_" are used to allow linking into program which
    already links with another version of libjpeg
  - Folder organization, cmake, visual studio, and arduino copy script as in other
    eosal/iocom libraries.
  - Only JPEG format support is compiled in.

  Copyright 2020 Pekka Lehtikoski. This file is part of the iocom project and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

  Copyright (C) 1991-1998, Thomas G. Lane. This file is derived from work of the Independent
  JPEG Group's software.

****************************************************************************************************
*/
#pragma once
#ifndef EOSAL_JPEG_
#define EOSAL_JPEG_
#include "eosalx.h"


/* If C++ compilation, all functions, etc. from this point on included headers are
   plain C and must be left undecorated.
 */
OSAL_C_HEADER_BEGINS

#if OSAL_USE_JPEG_LIBRARY
#include "extensions/jpeg/common/jinclude.h"
#include "extensions/jpeg/common/jpeglib.h"
#include "extensions/jpeg/common/jerror.h"		/* get library error codes too */

#endif

/* Flags for os_compress_JPEG() and os_uncompress_JPEG() functions.
 */
#define OSAL_JPEG_DEFAULT 0
#define OSAL_JPEG_SELECT_ALPHA_CHANNEL 1

/* Structure for managing uncompressing.
 */
typedef struct osalJpegMallocContext
{
    os_uchar *buf;
    os_memsz buf_sz;
    os_memsz row_nbytes;
    os_memsz nbytes;
    os_int w, h;
    osalBitmapFormat format;
}
osalJpegMallocContext;

#if OSAL_USE_JPEG_LIBRARY

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
    osalJpegMallocContext *alloc_context,
    os_int flags);


/* If C++ compilation, end the undecorated code.
 */
OSAL_C_HEADER_ENDS

#endif
#endif
