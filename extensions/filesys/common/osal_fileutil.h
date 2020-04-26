/**

  @file    filesys/common/osal_fileutil.h
  @brief   File helper functions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#if OSAL_FILESYS_SUPPORT

/* Flags for file utility functions.
 */
#define OS_FILE_DEFAULT 0
#define OS_FILE_NULL_CHAR 1


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
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags);

/* Read whole file into buffer allocated by os_malloc().
 */
os_char *os_read_file_alloc(
    const os_char *path,
    os_memsz *n_read,
    os_int flags);

/* Write whole file from buffer.
 */
osalStatus os_write_file(
    const os_char *path,
    const os_char *buf,
    os_memsz n,
    os_int flags);

/* Delete a files or directories with wildcard.
 */
osalStatus osal_remove_recursive(
    const os_char *path,
    const os_char *wildcard,
    os_int flags);

/*@}*/

#endif
