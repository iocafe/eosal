/**

  @file    filesys/windows/osal_filefstat.c
  @brief   Get file information.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#include <windows.h>

/**
****************************************************************************************************

  @brief Get file information, like time stamp, size, etc.
  @anchor osal_filestat

  The osal_filestat() function...

  @param  path Path to file or directory.
  @param  filestat Strucure to fill in with file stat information.
  @return If successfull, the function returns OSAL_SUCCESS(0). Other return values
          indicate an error.

****************************************************************************************************
*/
osalStatus osal_filestat(
    const os_char *path,
    osalFileStat *filestat)
{
    WIN32_FILE_ATTRIBUTE_DATA winfa;
    wchar_t *path_utf16;
    os_memsz path_sz;
    osalStatus rval = OSAL_SUCCESS;

    union
    {
        FILETIME winftime;
        os_int64 i64;
    }
    conv_union;

    /* Convert path and wild card to UTF16.
     */
    path_utf16 = osal_str_utf8_to_utf16_malloc(path, &path_sz);

    /* Get file stats from windows.
     */
    if (!GetFileAttributesExW(path_utf16, GetFileExInfoStandard, &winfa))
    {
        rval = OSAL_STATUS_FAILED;
    }
    else
    {
        filestat->isdir = (winfa.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? OS_TRUE : OS_FALSE;
        filestat->sz = (((os_long)winfa.nFileSizeHigh) << 32) | winfa.nFileSizeLow;

        conv_union.winftime = winfa.ftLastWriteTime;
        filestat->tstamp = conv_union.i64/10 - OSAL_WINDOWS_FILETIME_OFFSET;
   }    

    os_free(path_utf16, path_sz);
    return rval;
}
