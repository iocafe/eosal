/**

  @file    eosal_jpeg/eosal_code/osal_jpeg_dst.h
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

/* Prepare for JPEG compression destination.
 */
osalStatus osal_jpeg_setup_destination(
    void *cprm,
    osalStream dst_stream,
    os_uchar *dst_buf,
    os_memsz dst_buf_sz);

/* Release memory, get number of JPEG bytes and status code.
 */
osalStatus osal_jpeg_destination_finished(
    void *cprm,
    os_memsz *dst_nbytes);
