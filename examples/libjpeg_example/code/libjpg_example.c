/**

  @file    eosal/examples/libjpeg_example/code/libjpeg_example.c
  @brief   Example code about threads.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    5.6.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal_jpeg.h"

/* If needed for the operating system, EOSAL_C_MAIN macro generates the actual C main() function.
 */
EOSAL_C_MAIN


/**
****************************************************************************************************

  @brief Process entry point.

  The osal_main() function is OS independent entry point.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
osalStatus osal_main(
    os_int argc,
    os_char *argv[])
{
    /* When emulating micro-controller on PC, run loop. Just save context pointer on
       real micro-controller.
     */
    osal_simulated_loop(OS_NULL);
    return OSAL_SUCCESS;
}


osalStatus osal_loop(
    void *app_context)
{
    const os_int width = 100, height = 120;
    const osalBitmapFormat format = OSAL_RGB24;
    os_int row_nbytes = width * OSAL_BITMAP_BYTES_PER_PIX(format);
    os_int bytes = row_nbytes * height;
    os_uchar *buf;
    os_int i;
    osalStatus s;
    os_uchar *jpeg_buf;
    os_memsz jpeg_sz, jpeg_nbytes;
    osalJpegMallocContext alloc_context;
    jpeg_buf = (os_uchar*)os_malloc(0xFFFF, &jpeg_sz);

    buf = (os_uchar*)os_malloc(bytes, OS_NULL);
    for (i = 0; i<bytes; i++) {
        buf[i] = (os_uchar)i;
    }
    s = os_compress_JPEG(buf, width, height, row_nbytes, format, 50, OS_NULL,
        jpeg_buf, jpeg_sz, &jpeg_nbytes, OSAL_JPEG_DEFAULT);
    if (s) {
        osal_debug_error_int("os_compress_JPEG() failed s=", s);
    }
    else {
        osal_debug_error_int("compressed bytes ", jpeg_nbytes);
    }
    os_free(buf, bytes);

    os_memclear(&alloc_context, sizeof(alloc_context));
    s = os_uncompress_JPEG(jpeg_buf, jpeg_nbytes, &alloc_context, OSAL_JPEG_DEFAULT);
    if (s) {
        osal_debug_error_int("os_uncompress_JPEG() failed s=", s);
    }
    else {
        osal_debug_error_int("uncompressed bytes ", alloc_context.nbytes);
    }

    os_free(alloc_context.buf, alloc_context.buf_sz);

    os_free(jpeg_buf, jpeg_sz);
    os_sleep(10);
    return OSAL_SUCCESS;
}

/*  Empty function implementation needed to build for microcontroller.
 */
void osal_main_cleanup(
    void *app_context)
{

}
