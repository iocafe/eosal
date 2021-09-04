/**

  @file    jpeg/common/osal_jpeg_src.h
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

/* Prepare for JPEG uncompression.
 */
void osal_jpeg_setup_source(
    void *cprm,
    os_uchar *src_buf,
    os_memsz src_nbytes);
