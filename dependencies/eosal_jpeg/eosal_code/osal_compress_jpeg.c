/**

  @file    eosal_jpeg/eosal_code/osal_compress_jpeg.c
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
#if IOC_USE_JPEG_COMPRESSION
#include "code/jerror.h"
#include "eosal_code/osal_jpeg_dst.h"

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

  @brief Convert a bitmap in memory to JPEG.
  @anchor os_compress_JPEG

  The os_compress_JPEG() function stores resulting JPEG into stream or to buffer allocated by
  application.

  @param   src Source bitmap data.
  @param   w Source bitmap width in pixels.
  @param   h Source bitmap height in pixels.
  @param   row_nbytes Row width in bytes. This may be different from from width * pixel size,
           to align rows in specific ways or to compress just part of input bitmap.
  @param   format Source bitmap format, one of: OSAL_GRAYSCALE8, OSAL_GRAYSCALE16, OSAL_RGB24,
           or OSAL_RGBA32.
  @param   quality Compression quality, 0 - 100.
  @param   dst_stream Stream where to store the resulting JPEG. OS_NULL if storing JPEG to
           application allocated buffer.
  @param   dst_buf Pointer to buffer where to store resulting JPEG. OS_NULL if storing JPEG to
           stream.
  @param   dst_buf_sz Destination buffer size, if using allocated buffer. Ignored if
           compressing to a stream.
  @param   dst_nbytes Pointer where to store resulting JPEG size upon successful compression.
  @param   flags Bit fields, set OSAL_JPEG_DEFAULT (0) for default operation. Set
           OSAL_JPEG_SELECT_ALPHA_CHANNEL bit to save alpha channel of RGGB132
           bitmap (not yet implemented).

  @return  OSAL_SUCCESS if all is good. Other return values indicate an error.

****************************************************************************************************
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
    os_int flags)
{
    struct jpeg_compress_struct prm;            /* Compression parameters. */
    struct osalJpegErrorManager err_manager;    /* Own error manager */
    JSAMPROW row_pointer[1];                    /* Pointer source scanline. */
    os_uchar *s, *d, *b, *scanline;
    os_memsz b_sz;
    os_int count;
    osalStatus status = OSAL_SUCCESS, finish_status;

    *dst_nbytes = 0;

    /* Check arguments.
     */
    if (src == OS_NULL || w <= 0 || h <= 0 || row_nbytes <= 0) {
        osal_debug_error("compress JPEG: illegal argument.");
        return OSAL_STATUS_FAILED;
    }

    /* Set libjpeg error handling and override error_exit so that compression errors will
       cause osal_jpeg_disaster_exit to call longjump to return here.
     */
    os_memclear(&prm, sizeof(prm));
    prm.err = jpeg_std_error(&err_manager.pub);
    err_manager.pub.error_exit = osal_jpeg_disaster_exit;
    if (setjmp(err_manager.jump_buffer)) {
        jpeg_destroy_compress(&prm);
        osal_debug_error("compress JPEG: compression failed.");
        return OSAL_STATUS_FAILED;
    }

    /* Set up the JPEG compression state structure and destination.
     */
    jpeg_create_compress(&prm);
    status = osal_jpeg_setup_destination(&prm, dst_stream, dst_buf, dst_buf_sz);
    if (status) {
        jpeg_destroy_compress(&prm);
        return OSAL_STATUS_FAILED;
    }

    /* Store image size.
     */
    prm.image_width = w;
    prm.image_height = h;

    /* Set up number of input components and the color space. Image is encoded as 8 or 16
       bit per pixel grayscale image or 24 bit color image.
     */
    switch (format)
    {
        /* 8 or 16 bit per pixel grayscale image, one color component per pixel.
         */
        case OSAL_GRAYSCALE8:
        case OSAL_GRAYSCALE16:
            prm.input_components = 1;
            prm.in_color_space = JCS_GRAYSCALE;
            break;

        /* 24 or 32 bit/pixel collor image, three color components per pixel.
         */
        case OSAL_RGB32:
        case OSAL_RGBA32:
            if (flags & OSAL_JPEG_SELECT_ALPHA_CHANNEL) {
                prm.input_components = 1;
                prm.in_color_space = JCS_GRAYSCALE;
                break;
            }
            /* continues... */

        case OSAL_RGB24:
            prm.input_components = 3;
            prm.in_color_space = JCS_RGB;
            break;

        default:
            osal_debug_error("compress JPEG: unsupported image format.");
            osal_jpeg_destination_finished(&prm, dst_nbytes);
            jpeg_destroy_compress(&prm);
            return OSAL_STATUS_FAILED;
    }

    /* Initialize prm with default compression parameters and set
       compression quality.
     */
    jpeg_set_defaults(&prm);
    jpeg_set_quality(&prm, quality, TRUE);

    /* Start compressor. TRUE ensures that we will write
       a complete interchange-JPEG format.
     */
    jpeg_start_compress(&prm, TRUE);

    /* Create the JPEG. The Library's variable prm.next_scanline is used as a loop counter.
       One scan line at a time to keep things simple.
     */
    switch (format)
    {
        /* 8 bit grayscale formats can be writted directly.
         */
        default:
        case OSAL_GRAYSCALE8:
            while (prm.next_scanline < prm.image_height)
            {
                row_pointer[0] = (os_char*)(src + prm.next_scanline * (os_memsz)row_nbytes);
                jpeg_write_scanlines(&prm, row_pointer, 1);
            }
            break;

        /* 16 bit/pixel grayscale image
         */
        case OSAL_GRAYSCALE16:
            /* Allocate buffer to convert a scan line to 8 bit grayscale.
             */
            b = (os_uchar*)os_malloc(w, OS_NULL);
            if (b == OS_NULL)
            {
                status = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
                goto getout;
            }
#if OSAL_SMALL_ENDIAN
            scanline = src + 1;
#else
            scanline = src;
#endif

            /* Go through image, scan line at a time.
             */
            while (prm.next_scanline < prm.image_height)
            {
                /* Convert a scan line from 16 bit grayscale to 8 bit grayscale.
                 */
                count = w;
                d = b;
                s = scanline;

                /* Use only more significant bytes.
                 */
                while (count--)
                {
                    *(d++) = *s;
                    s += 2;
                }

                /* Compress scan line.
                 */
                row_pointer[0] = (os_char*)b;
                jpeg_write_scanlines(&prm, row_pointer, 1);

                /* Advance to next scan line.
                 */
                scanline += row_nbytes;
            }

            /* Release scan line buffer.
             */
            os_free(b, w);
            break;

        /* 24 bit/pixel color image
         */
        case OSAL_RGB24:
#if OSAL_BGR_COLORS
            /* Allocate buffer to convert a scan line to 24 bit RGB.
             */
            b_sz = 3 * (os_memsz)w;
            b = (os_uchar*)os_malloc(b_sz, OS_NULL);
            if (b == OS_NULL)
            {
                status = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
                goto getout;
            }
            scanline = src;

            /* Go through image, scan line at a time.
             */
            while (prm.next_scanline < prm.image_height)
            {
                /* Convert a scan line from 16 bit RGBA to 24 bit RGB.
                 */
                count = w;
                d = b;
                s = scanline;

                /* Copy RGB info (either RGB or BGR order).
                 */
                while (count--) {
                    d[2] = *(s++); d[1] = *(s++); d[0] = *(s++); d += 3;
                }

                /* Compress a scan line and move to next one.
                 */
                row_pointer[0] = (os_char*)b;
                jpeg_write_scanlines(&prm, row_pointer, 1);
                scanline += row_nbytes;
            }

            /* Release scan line buffer.
             */
            os_free(b, b_sz);
            break;
#else
            /* Go through image, compress one scan line at a time.
             */
            scanline = src;
            while (prm.next_scanline < prm.image_height) {
                jpeg_write_scanlines(&prm, &scanline, 1);
                scanline += row_nbytes;
            }
            break;
#endif
        /* 32 bit/pixel color image with or without alpha channel.
         */
        case OSAL_RGB32:
        case OSAL_RGBA32:
            /* Allocate buffer to convert a scan line to 24 bit RGB.
             */
            b_sz = (flags & OSAL_JPEG_SELECT_ALPHA_CHANNEL) ? w : 3 * (os_memsz)w;
            b = (os_uchar*)os_malloc(b_sz, OS_NULL);
            if (b == OS_NULL)
            {
                status = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
                goto getout;
            }
            scanline = src;
            if (flags & OSAL_JPEG_SELECT_ALPHA_CHANNEL) scanline += 3;

            /* Go through image, scan line at a time.
             */
            while (prm.next_scanline < prm.image_height)
            {
                /* Convert a scan line from 16 bit RGBA to 24 bit RGB.
                 */
                count = w;
                d = b;
                s = scanline;

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
                        s++;
                    }
                }

                /* Compress scan line and move to next one.
                 */
                row_pointer[0] = (os_char*)b;
                jpeg_write_scanlines(&prm, row_pointer, 1);
                scanline += row_nbytes;
            }

            /* Release scan line buffer.
             */
            os_free(b, b_sz);
            break;
    }

    /* Finished.
     */
getout:
    jpeg_finish_compress(&prm);
    finish_status = osal_jpeg_destination_finished(&prm, dst_nbytes);
    if (finish_status) status = finish_status;
    jpeg_destroy_compress(&prm);
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
