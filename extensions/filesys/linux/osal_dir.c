/**

  @file    filesys/linux/osal_dir.c
  @brief   Directory related functions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Functions for listing, creating and removing a directory.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_FILESYS_SUPPORT

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

/**
****************************************************************************************************

  @brief List directory. Allocates directory list list.
  @anchor osal_dir

  The osal_dir() function...

  @param  path Path to directory.
  @param  wildcard Like "*.txt".
  @param  list Where to store pointer to first item of directory list. The memory for list
          needs to be released by calling osal_free_dirlist().
  @param  flags Set OSAL_DIR_DEFAULT (0) for simple operation. OSAL_DIR_FILESTAT to get
          also file size, isdir type and time stamp.
  @return If successfull, the function returns OSAL_SUCCESS(0). Other return values
          indicate an error.

****************************************************************************************************
*/
osalStatus osal_dir(
    const os_char *path,
    const os_char *wildcard,
    osalDirListItem **list,
    os_int flags)
{
    struct dirent *pDirent;
    DIR *pDir;
    osalDirListItem *item, *prev;
    os_memsz len;
    osalFileStat filestat;
    os_char *fspath = OS_NULL;
    os_memsz fspath_sz = 0;
    os_memsz fspath_pos = 0;

    /* In case of errors.
     */
    *list = OS_NULL;

    pDir = opendir(path);
    if (pDir == NULL)
    {
        switch (errno)
        {
            case EACCES:
                return OSAL_STATUS_NO_ACCESS_RIGHT;

            default:
                return OSAL_STATUS_FAILED;
        }
    }

    if (flags & OSAL_DIR_FILESTAT)
    {
        fspath_pos = os_strlen(path) - 1;
        fspath = os_malloc(fspath_pos + 64, &fspath_sz);
        os_strncpy(fspath, path, fspath_sz);
        if (fspath_pos > 0) if (fspath[fspath_pos-1] != '/')
        {
            fspath[fspath_pos++] = '/';
        }
    }

    prev = OS_NULL;
    while ((pDirent = readdir(pDir)) != NULL)
    {
        /* Skip ones which do not match wildcard.
         */
        if (!osal_pattern_match(pDirent->d_name, wildcard, 0)) continue;

        /* Allocate empty item and join it to chain.
         */
        item = (osalDirListItem*)os_malloc(sizeof(osalDirListItem), OS_NULL);
        os_memclear(item, sizeof(osalDirListItem));
        if (prev) prev->next = item;
        else *list = item;
        prev = item;
        len = os_strlen(pDirent->d_name);

        if (flags & OSAL_DIR_FILESTAT)
        {
            /* If we need to make path buffer longer
             */
            if (fspath_pos + len >= fspath_sz)
            {
                os_free(fspath, fspath_sz);
                fspath_pos = os_strlen(path) - 1;
                fspath = os_malloc(fspath_pos + len + 64, &fspath_sz);
                os_strncpy(fspath, path, fspath_sz);
                if (fspath_pos > 0) if (fspath[fspath_pos-1] != '/')
                {
                    fspath[fspath_pos++] = '/';
                }
            }

            os_memcpy(fspath + fspath_pos, pDirent->d_name, len);
            if (!osal_filestat(path, &filestat))
            {
                item->isdir = filestat.isdir;
                item->sz = filestat.sz;
                osal_int64_copy(&item->tstamp, &filestat.tstamp);
            }
        }

        /* Allocate memory and save file name.
         */
        item->name = os_malloc(len, OS_NULL);
        os_memcpy(item->name, pDirent->d_name, len);
    }

    closedir (pDir);

    os_free(fspath, fspath_sz);

    return OSAL_SUCCESS;
}

/**
****************************************************************************************************

  @brief Release directory list from memory.
  @anchor osal_free_dirlist

  The osal_free_dirlist() function releases memory allocated for list items and file name strings.

  @param  list Pointer to first item of list to remove.
  @return None.

****************************************************************************************************
*/
void osal_free_dirlist(
    osalDirListItem *list)
{
    osalDirListItem *item, *nextitem;

    for (item = list; item; item = nextitem)
    {
        nextitem = item->next;

        os_free(item->name, os_strlen(item->name));
        os_free(item, sizeof(osalDirListItem));
    }
}


/**
****************************************************************************************************

  @brief Create directory.
  @anchor osal_mkdir

  The osal_mkdir() function...

  @param  path Path to directory.
  @param  flags Reserved for future, set zero for now.
  @return If successfull, the function returns OSAL_SUCCESS (0). Other return values
          indicate an error.

****************************************************************************************************
*/
osalStatus osal_mkdir(
    const os_char *path,
    os_int flags)
{
    /* Create directory with read/write/search permissions for everybody.
     */
    if (mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO))
    {
        switch (errno)
        {
            /* Process does not have rights to create directory.
             */
            case EACCES:
                return OSAL_STATUS_NO_ACCESS_RIGHT;

            /* Directory already exists. This is not treated as an error.
             */
            case EEXIST:
                break;

            /* Other errors.
             */
            default:
                return OSAL_STATUS_FAILED;
        }
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Remove directory.
  @anchor osal_rmdir

  The osal_rmdir() function...

  @param  path Path to directory.
  @param  flags Reserved for future, set zero for now.
  @return If successfull, the function returns OSAL_SUCCESS(0). Other return values
          indicate an error, specifically OSAL_DIR_NOT_EMPTY means that directory is not empty.

****************************************************************************************************
*/
osalStatus osal_rmdir(
    const os_char *path,
    os_int flags)
{
    if (rmdir(path))
    {
        switch (errno)
        {
            /* Process does not have rights to create directory.
             */
            case EACCES:
            case EPERM:
                return OSAL_STATUS_NO_ACCESS_RIGHT;

            /* Directory is not empty.
             */
            case EEXIST:
            case ENOTEMPTY:
                return OSAL_DIR_NOT_EMPTY;

            /* Other errors.
             */
            default:
                return OSAL_STATUS_FAILED;
        }
    }

    return OSAL_SUCCESS;
}

#endif
