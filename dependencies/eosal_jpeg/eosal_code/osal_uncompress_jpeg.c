/**

  @file    eosal_jpeg/eosal_code/osal_uncompress_jpeg.c
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
#include "code/jerror.h"
#include "eosal_code/osal_jpeg_src.h"

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
  @param   dst_nbytes Pointer where to store resulting JPEG size upon successfull compression.
  @param   alloc_func Pointer to function to allocate buffer within allocation context. Can
           be OS_NULL if begger is pre allocated (buf and buf_sz set), or to allocate
           buffer by os_malloc(). In latter case buffer must be release by application.
  @oaram   alloc_context Sturcture for managing allocation. Clear before calling this structure
           and optionally set buf and buf_sz.
  @param   flags Bit fields, set OSAL_JPEG_DEFAULT (0) for default operation. Set
           OSAL_JPEG_SELECT_ALPHA_CHANNEL bit to save alpha channel of RGBA32
           bitmap (not yet implemented).

  @return  OSAL_SUCCESS if all is good. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus os_uncompress_JPEG(
    os_uchar *src_buf,
    os_memsz src_nbytes,
    osal_jpeg_malloc_func *alloc_func,
    osalJpegMallocContext *alloc_context,
    os_int flags)
{
    struct jpeg_decompress_struct prm;          /* Decompression parameters */
    struct osalJpegErrorManager err_manager;    /* Own error manager */
    os_uchar *d, *s, *p, *b, *scanline, *dst_buf;
    os_memsz sz;
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
    format = prm.num_components == 1 ? OSAL_GRAYSCALE8 : OSAL_RGB24,

    sz = w * h * prm.num_components;

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
    alloc_context->format = format;
    alloc_context->nbytes = sz;

    if (alloc_func) {
        status = alloc_func(alloc_context, sz);
        if (status) goto getout;
    }
    else if (alloc_context->buf == OS_NULL)
    {
        alloc_context->buf = (os_uchar*)os_malloc(sz, OS_NULL);
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
                p += w;
            }
            break;

        /* 24 bit/pixel RGB image
         */
        case OSAL_RGB24:
            /* Allocate buffer for reading 24 bit RGB data.
             */
            b = (os_uchar*)os_malloc(3 * w, OS_NULL);
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

                while (count--)
                {
#if OE_BGR_COLORS
                    d[2] = *(s++); d[1] = *(s++); d[0] = *(s++); d += 3;
#else
                    *(d++) = *(s++); *(d++) = *(s++); *(d++) = *(s++);
#endif
                }

                /* Advance to next scan line.
                 */
                scanline += 3 * w;
            }

            os_free(b, 3 * w);
            break;

        /* 32 bit/pixel RGB image with alpha channel
         */
        case OSAL_RGBA32:
            /* Allocate buffer for reading 24 bit RGB data.
             */
            b = (os_uchar*)os_malloc(3 * w, OS_NULL);
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

                while (count--)
                {
#if OE_BGR_COLORS
                    d[2] = *(s++); d[1] = *(s++); d[0] = *(s++); d += 3;
#else
                    *(d++) = *(s++); *(d++) = *(s++); *(d++) = *(s++);
#endif
                    *(d++) = 0xFF;
                }

                /* Next scan line.
                 */
                scanline += 4 * w;
            }

            os_free(b, 3 * w);
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
  @anchor os_compress_JPEG

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
