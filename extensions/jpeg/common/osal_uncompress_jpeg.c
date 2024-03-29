/**

  @file    jpeg/common/osal_uncompress_jpeg.c
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
#include "eosalx.h"
#if OSAL_USE_JPEG_LIBRARY
#include "extensions/jpeg/common/jerror.h"
#include "extensions/jpeg/common/osal_jpeg_src.h"

#include <setjmp.h>

/** Libjpeg's error handler structure, extended by setjump buffer. C standard library setjmp/longjmp
    mechanism is used to return control to the calling function if JPEG compression or decompression
    fails. This sturcture started with default libjpeg fields "buf" followed by application specific
    jump_buffer to exit the compression.
 */
typedef struct osalJpegErrorManager
{
    struct jpeg_error_mgr pub;
    jmp_buf jump_buffer;
}
osalJpegErrorManager;

/* Forward referred static function.
 */
static void osal_jpeg_disaster_exit(
    j_common_ptr ptr);


/**
****************************************************************************************************

  @brief Uncompress JPEG to bitmap in memory.
  @anchor os_uncompress_JPEG

  The os_uncompress_JPEG() function stores resulting bitmap...

  @param   src_buf Pointer to source JPEG data.
  @param   src_nbytes Size of JPEG data in bytes.
  @param   dst_nbytes Pointer where to store resulting JPEG size upon successful compression.
  @param   alloc_context Sturcture for managing allocation. Clear before calling this structure
           and optionally set buf and buf_sz.
           Calling application can optionally set row_nbytes and format in alloc_context
           structure: row_nbytes is needed bitmap rows are aligned to even, etc memory addressess,
           or we are uncompressin smalled jpeg inside larger buffer. The format is
           needed when uncompressing color image into OSAL_RGBA32 bitmap where alpha
           channel is compressed separately.
           If buffer is allocated by this function, it must be freed by os_free().
  @param   flags Bit fields, set OSAL_JPEG_DEFAULT (0) for default operation. Set
           OSAL_JPEG_SELECT_ALPHA_CHANNEL bit to save alpha channel of RGBA32
           bitmap (not yet implemented).

  @return  OSAL_SUCCESS if all is good. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus os_uncompress_JPEG(
    os_uchar *src_buf,
    os_memsz src_nbytes,
    osalJpegMallocContext *alloc_context,
    os_int flags)
{
    struct jpeg_decompress_struct prm;          /* Decompression parameters */
    struct osalJpegErrorManager err_manager;    /* Own error manager */
    os_uchar *d, *s, *p, *b, *scanline, *dst_buf;
    os_memsz sz, row_nbytes, b_sz;
    os_int count, w, h;
    // os_ushort v;
    osalStatus status = OSAL_SUCCESS;
    osalBitmapFormat format;

    /* Check arguments.
     */
    if (src_buf == OS_NULL) {
        osal_debug_error("compress JPEG: illegal argument.");
        return OSAL_STATUS_FAILED;
    }

    /* Set libjpeg error handling and override error_exit so that compression errors will
       cause osal_jpeg_disaster_exit to call longjump to return here.
     */
    os_memclear(&prm, sizeof(prm));
    prm.err = jpeg_std_error(&err_manager.pub);
    err_manager.pub.error_exit = osal_jpeg_disaster_exit;
    if (setjmp(err_manager.jump_buffer))
    {
        jpeg_destroy_decompress(&prm);
        osal_debug_error("compress JPEG: compression failed.");
        return OSAL_STATUS_FAILED;
    }

    /* Initialize for decompressing.
     */
    jpeg_create_decompress(&prm);

    /* Set up the data source (where to get JPEG data), either stream or memory buffer.
     */
    osal_jpeg_setup_source(&prm, src_buf, src_nbytes);

    /* Read JPEG parameters with jpeg_read_header()
     */
    jpeg_read_header(&prm, TRUE);

    /* Start decompressor.
     */
    jpeg_start_decompress(&prm);

    w = prm.output_width;
    h = prm.output_height;
    format = alloc_context->format;
    if (format == 0) {
        format = prm.num_components == 1 ? OSAL_GRAYSCALE8 : OSAL_RGB24;
        alloc_context->format = format;
    }

    row_nbytes = alloc_context->row_nbytes;
    if (row_nbytes == 0) {
        row_nbytes = w * OSAL_BITMAP_BYTES_PER_PIX(alloc_context->format);
        alloc_context->row_nbytes = row_nbytes;
    }

    sz = (os_memsz)h * (os_memsz)row_nbytes;

    /* Verify that parameters within JPEG are correct.
     */
    if (prm.output_height <= 0 ||
        prm.output_width  <= 0)
    {
        osal_debug_error("Errornous JPEG data");
        status = OSAL_STATUS_FAILED;
        goto getout;
    }

    alloc_context->w = w;
    alloc_context->h = h;
    alloc_context->nbytes = sz;

    if (alloc_context->buf == OS_NULL)
    {
        alloc_context->buf = (os_uchar*)os_malloc(sz, &alloc_context->buf_sz);
    }
    dst_buf = alloc_context->buf;
    if (dst_buf == OS_NULL)
    {
        status = OSAL_STATUS_FAILED;
        goto getout;
    }

    /* Uncompress and place into bitmap.
     */
    switch (format)
    {
        /* 8 bit/pixel grayscale image
         */
        case OSAL_GRAYSCALE8:
            p = dst_buf;
            while (prm.output_scanline < prm.output_height)
            {
                jpeg_read_scanlines(&prm, &p, 1);
                p += row_nbytes;
            }
            break;

        /* 24 bit/pixel RGB image
         */
        case OSAL_RGB24:
#if OSAL_BGR_COLORS
            /* Allocate buffer for reading 24 bit RGB data.
             */
            b_sz = 3 * (os_memsz)w;
            b = (os_uchar*)os_malloc(b_sz, OS_NULL);
            if (b == OS_NULL)
            {
                status = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
                goto getout;
            }
            scanline = dst_buf;

            /* Go through image, scan line at a time.
             */
            while (prm.output_scanline < prm.output_height)
            {
                jpeg_read_scanlines(&prm, &b, 1);

                count = w;
                d = scanline;
                s = b;

                while (count--) {
                    d[2] = *(s++); d[1] = *(s++); d[0] = *(s++); d += 3;
                }

                /* Advance to next scan line.
                 */
                scanline += row_nbytes;
            }

            os_free(b, b_sz);
            break;
#else
            /* Go through image, uncompress one scan line at a time.
             */
            scanline = dst_buf;
            while (prm.output_scanline < prm.output_height) {
                jpeg_read_scanlines(&prm, &scanline, 1);
                scanline += row_nbytes;
            }
            break;
#endif

        /* 32 bit/pixel RGB image with or without alpha channel
         */
        case OSAL_RGB32:
        case OSAL_RGBA32:
            /* Allocate buffer for reading 24 bit RGB data.
             */
            b_sz = (flags & OSAL_JPEG_SELECT_ALPHA_CHANNEL) ? w : 3 * (os_memsz)w;
            b = (os_uchar*)os_malloc(b_sz, OS_NULL);
            if (b == OS_NULL)
            {
                status = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
                goto getout;
            }
            scanline = dst_buf;
            if (flags & OSAL_JPEG_SELECT_ALPHA_CHANNEL) scanline += 3;

            /* Go through image, scan line at a time.
             */
            while (prm.output_scanline < prm.output_height)
            {
                jpeg_read_scanlines(&prm, &b, 1);

                count = w;
                d = scanline;
                s = b;

                /* Copy RGB info (either RGB or BGR order), skip alpha channnel.
                 */
                if (flags & OSAL_JPEG_SELECT_ALPHA_CHANNEL) {
                    while (count--)
                    {
                        *(d++) = *s;
                        s += 4;
                    }
                }

                else {
                    while (count--)
                    {
#if OSAL_BGR_COLORS
                        d[2] = *(s++); d[1] = *(s++); d[0] = *(s++); d += 3;
#else
                        *(d++) = *(s++); *(d++) = *(s++); *(d++) = *(s++);
#endif
                        *(d++) = 0xFF;
                    }
                }

                /* Next scan line.
                 */
                scanline += row_nbytes;
            }

            os_free(b, b_sz);
            break;

        default:
            osal_debug_error("uncompress JPEG: unsupported image format.");
            break;
    }

    /* Finished.
     */
getout:
    jpeg_finish_decompress(&prm);
    jpeg_destroy_decompress(&prm);
    return status;
}


/**
****************************************************************************************************

  @brief Jump to exit compression/decompression on error.
  @anchor osal_jpeg_disaster_exit

  This function is called by libjpeg if compression or decompression fails. It calls longjump
  to terminate compression/decompression.

  @param   ptr libjpg common pointer.
  @return  None

****************************************************************************************************
*/
static void osal_jpeg_disaster_exit(
    j_common_ptr ptr)
{
    osalJpegErrorManager *err_manager;

    err_manager = (osalJpegErrorManager*) ptr->err;
    longjmp(err_manager->jump_buffer, 1);
}

#endif