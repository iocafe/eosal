/**

  @file    filesys/linux/osal_filectrl.c
  @brief   File helper functions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#ifdef OSAL_LINUX
#if OSAL_FILESYS_SUPPORT
#include <stdio.h>
#include <errno.h>


/**
****************************************************************************************************

  @brief Delete a file.
  @anchor osal_remove

  The osal_remove() function deletes one file. Do not use this function for recursively or
  with wild cards.

  @param  path Path to file.
  @param  flags Reserved for future, set zero for now.
  @return If successful, the function returns OSAL_SUCCESS(0). Other return values
          indicate an error, for example:
          - OSAL_STATUS_FILE_DOES_NOT_EXIST: File doesn't exist.
          - OSAL_STATUS_NO_ACCESS_RIGHT: User doesn't have rights to delete the file.

****************************************************************************************************
*/
osalStatus osal_remove(
    const os_char *path,
    os_int flags)
{
    if (remove(path))
    {
        switch (errno)
        {
            /* Process does not have rights to delete the file.
             */
            case EACCES:
            case EPERM:
            case EROFS:
                return OSAL_STATUS_NO_ACCESS_RIGHT;

            /* File desn't exist.
             */
            case ENOENT:
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
#endif
