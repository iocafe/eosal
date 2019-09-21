/**

  @file    filesys/common/osal_fileutil.h
  @brief   File helper functions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.9.2019

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_FILEUTIL_INCLUDED
#define OSAL_FILEUTIL_INCLUDED
#if OSAL_FILESYS_SUPPORT


/** 
****************************************************************************************************

  @name File helper functions.

  Read and write whole file with one function call.

****************************************************************************************************
 */
/*@{*/

/* Read whole file into buffer.
 */
osalStatus os_read_file(
    const os_char *path,
    os_uchar *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags);

/* Read whole file into buffer allocated by os_malloc().
 */
os_uchar *os_read_file_alloc(
    const os_char *path,
    os_memsz *n_read,
    os_int flags);

/* Write whole file from buffer.
 */
osalStatus os_write_file(
    const os_char *path,
    const os_uchar *buf,
    os_memsz n,
    os_int flags);

/*@}*/

#endif
#endif
