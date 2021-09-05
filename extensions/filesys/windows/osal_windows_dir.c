/**

  @file    filesys/windows/osal_dir.c
  @brief   Directory related functions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Functions for listing, creating and removing a directory.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#ifdef OSAL_WINDOWS

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
  @return If successful, the function returns OSAL_SUCCESS(0). Other return values
          indicate an error.

****************************************************************************************************
*/
osalStatus osal_dir(
    const os_char *path,
    const os_char *wildcard,
    osalDirListItem **list,
    os_int flags)
{
    HANDLE handle;
    WIN32_FIND_DATAW finddata;
    osalDirListItem *item, *prev;
    os_memsz len;
    wchar_t *path_utf16, *wildcard_utf16, *fspath;
    os_memsz path_sz, wildcard_sz, sz, fspath_sz;
    os_memsz fspath_pos;
    os_int64 i64;
    union
    {
        FILETIME winftime;
        os_int64 i64;
    }
    conv_union;
    osalStatus rval = OSAL_SUCCESS;

    /* In case of errors.
     */
    *list = OS_NULL;

    /* Convert path and wild card to UTF16.
     */
    path_utf16 = osal_str_utf8_to_utf16_malloc(path, &path_sz);
    wildcard_utf16 = osal_str_utf8_to_utf16_malloc(path, &wildcard_sz);

    /* Allocate buffer
     */
    len = wcslen(wildcard_utf16) + 1;
    fspath_pos = wcslen(path_utf16);
    fspath = (wchar_t*)os_malloc((fspath_pos + len + 2)*sizeof(wchar_t), &sz);
    fspath_sz = sz/sizeof(wchar_t);
    wcscpy_s(fspath, (rsize_t)fspath_sz, path_utf16);
    if (fspath_pos > 0) if (fspath[fspath_pos-1] != '/')
    {
        fspath[fspath_pos++] = '/';
    }
    wcscpy_s(fspath + fspath_pos, (rsize_t)(fspath_sz - fspath_pos), wildcard_utf16);

    handle = FindFirstFileW(fspath, &finddata);
    if  (handle != INVALID_HANDLE_VALUE) 
    {
        rval = OSAL_STATUS_FAILED;
        goto getout;
    }

    prev = OS_NULL;
    do {
        /* Allocate empty item and join it to chain.
         */
        item = (osalDirListItem*)os_malloc(sizeof(osalDirListItem), OS_NULL);
        os_memclear(item, sizeof(osalDirListItem));
        if (prev) prev->next = item;
        else *list = item;
        prev = item;

        if (flags & OSAL_DIR_FILESTAT)
        {
            item->isdir = (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? OS_TRUE : OS_FALSE;
       
            osal_int64_set_uint2(&i64, finddata.nFileSizeLow, finddata.nFileSizeHigh);
            item->sz = osal_int64_get_long(&i64);

            conv_union.winftime = finddata.ftLastWriteTime;
            item->tstamp = conv_union.i64/10 - OSAL_WINDOWS_FILETIME_OFFSET;
        }

        /* Allocate memory and save file name as UTF8.
         */
        item->name = osal_str_utf16_to_utf8_malloc(finddata.cFileName, OS_NULL);
    }
    while (FindNextFileW(handle, &finddata) != 0);

    if (GetLastError() != ERROR_NO_MORE_FILES) 
    {
        rval = OSAL_STATUS_FAILED;
    }
 
    FindClose(handle);

getout:
    os_free(fspath, sz);

    os_free(path_utf16, path_sz);
    os_free(wildcard_utf16, wildcard_sz);

    return rval;
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
  @return If successful, the function returns OSAL_SUCCESS (0). Other return values
          indicate an error.

****************************************************************************************************
*/
osalStatus osal_mkdir(
    const os_char *path,
    os_int flags)
{
    wchar_t *path_utf16;
    os_memsz path_sz;
    osalStatus rval = OSAL_SUCCESS;

    /* Convert path and wild card to UTF16.
     */
    path_utf16 = osal_str_utf8_to_utf16_malloc(path, &path_sz);

    /* Create directory.
     */
    if (!CreateDirectoryW(path_utf16, NULL))
    {
        /* Directory already exists is not treated as an error.
         */
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            rval = OSAL_STATUS_FAILED;
        }
    }

    os_free(path_utf16, path_sz);
    return rval;
}


/**
****************************************************************************************************

  @brief Remove directory.
  @anchor osal_rmdir

  The osal_rmdir() function removes a directory. The directory must be empty.

  @param  path Path to directory.
  @param  flags Reserved for future, set zero for now.
  @return If successful, the function returns OSAL_SUCCESS(0). Other return values indicate
          an error, specifically OSAL_STATUS_DIR_NOT_EMPTY means that directory is not empty.

****************************************************************************************************
*/
osalStatus osal_rmdir(
    const os_char *path,
    os_int flags)
{
    wchar_t *path_utf16;
    os_memsz path_sz;
    osalStatus rval = OSAL_SUCCESS;

    /* Convert path and wildcard to UTF16.
     */
    path_utf16 = osal_str_utf8_to_utf16_malloc(path, &path_sz);

    /* Remove directory.
     */
    if (!RemoveDirectoryW(path_utf16))
    {
        switch (GetLastError())
        {
            case ERROR_DIR_NOT_EMPTY:
                rval = OSAL_STATUS_DIR_NOT_EMPTY;
                break;

            default:
                rval = OSAL_STATUS_FAILED;
                break;
        }
    }

    os_free(path_utf16, path_sz);
    return rval;
}

#endif
