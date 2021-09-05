/**

  @file    filesys/linux/osal_filefstat.c
  @brief   Get file information.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#ifdef OSAL_LINUX
#if OSAL_FILESYS_SUPPORT

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// #include <errno.h>

/**
****************************************************************************************************

  @brief Get file information, like time stamp, size, etc.
  @anchor osal_filestat

  The osal_filestat() function...

  @param  path Path to file or directory.
  @param  filestat Strucure to fill in with file stat information.
  @return If successful, the function returns OSAL_SUCCESS(0). Other return values
          indicate an error.

****************************************************************************************************
*/
osalStatus osal_filestat(
    const os_char *path,
    osalFileStat *filestat)
{
    struct stat osfstat;

    /* Clear here, in case new items are added to structure and this implementation
       is not updated.
     */
    os_memclear(filestat, sizeof(osalFileStat));

    /* Request file information from operating system.
     */
    if (stat(path, &osfstat))
    {
        return OSAL_STATUS_FAILED;
    }

    /* If this is directory
     */
    filestat->isdir = S_ISDIR(osfstat.st_mode) ? OS_TRUE : OS_FALSE;

    /* File size in bytes.
     */
    filestat->sz = osfstat.st_size;

    /* Modification time stamp in microseconds.
     */
    osal_int64_set_long(&filestat->tstamp, osfstat.st_mtime);
    osal_int64_multiply(&filestat->tstamp, &osal_int64_1000000);

    return OSAL_SUCCESS;
}

#endif
#endif
