/**

  @file    filesys/windows/osal_filectrl.c
  @brief   File helper functions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    27.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_FILESYS_SUPPORT
#include <Windows.h>

/**
****************************************************************************************************

  @brief Delete a file.
  @anchor osal_remove

  The osal_remove() function deletes one file. Do not use this function for recursively or
  with wild cards.

  @param  path Path to file.
  @param  flags Reserved for future, set zero for now.
  @return If successfull, the function returns OSAL_SUCCESS(0). Other return values
          indicate an error, for example:
          - OSAL_STATUS_FILE_DOES_NOT_EXIST: File doesn't exist.
          - OSAL_STATUS_NO_ACCESS_RIGHT: User doesn't have rights to delete the file.

****************************************************************************************************
*/
osalStatus osal_remove(
    const os_char *path,
    os_int flags)
{
    wchar_t *path_utf16;
    os_memsz sz;
    BOOL ok;
    DWORD eno;

    /* Delete the file. Convert path from UTF8 to UTF16 string, allocate new buffer.
       Release buffer after open.
     */
    path_utf16 = osal_str_utf8_to_utf16_malloc(path, &sz);
    ok = DeleteFile(path_utf16);
    os_free(path_utf16, sz);

    if (!ok)
    {
        eno = GetLastError();
        switch (eno)
        {
            /* Process does not have rights to delete the file.
             */
            case ERROR_ACCESS_DENIED:
                return OSAL_STATUS_NO_ACCESS_RIGHT;

            /* File desn't exist.
             */
            case ERROR_FILE_NOT_FOUND:
                return OSAL_STATUS_FILE_DOES_NOT_EXIST;

            /* Other errors.
             */
            default:
                return OSAL_STATUS_FAILED;
        }
    }

    return OSAL_SUCCESS;
}

#endif
