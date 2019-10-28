/**

  @file    filesys/common/osal_fileutil.c
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
#include "eosalx.h"
#if OSAL_FILESYS_SUPPORT


/**
****************************************************************************************************

  @brief Read whole file into buffer.
  @anchor os_read_file

  The os_read_file() function reads a whole file into user given buffer.

  @param   path Path to the file.
  @param   buf Pointer to the buffer.
  @param   n Maximum number of bytes to read.
  @param   n_read Pointer to integer where to store number of bytes read.
  @param   flags OS_FILE_NULL_CHAR to terminate buffer with NULL character.
           OS_FILE_DEFAULT for default operation.
  @return  OSAL_SUCCESS if all good. Value OSAL_OUT_OF_BUFFER indicates that the file is larger
           than the buffer (file content still read). Other values indicate an error.

****************************************************************************************************
*/
osalStatus os_read_file(
    const os_char *path,
    os_uchar *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags)
{
    osalStream f;
    osalStatus s;
    os_memsz onemore;
    os_uchar tmp[1];

    *n_read = 0;

    /* If we want to have terminating NULL character, leave space for it.
     */
    if (flags & OS_FILE_NULL_CHAR) n--;

    /* Open file.
     */
    f = osal_file_open(path, OS_NULL, &s, OSAL_STREAM_READ);
    if (f == OS_NULL) return s;

    /* Read up to n bytes.
     */
    s = osal_file_read(f, buf, n, n_read, OSAL_STREAM_DEFAULT);
    if (s) goto getout;

    /* If we got n bytes, check if there is still more which doesn't fit to buffer.
     */
    if (*n_read == n)
    {
        osal_file_read(f, tmp, sizeof(tmp), &onemore, OSAL_STREAM_DEFAULT);
        if (onemore > 0) s = OSAL_OUT_OF_BUFFER;
    }

    /* Terminate with NULL character.
     */
    if (flags & OS_FILE_NULL_CHAR)
    {
        buf[(*n_read)++] = '\0';
    }

getout:
    osal_file_close(f);
    return s;
}


/**
****************************************************************************************************

  @brief Read whole file into buffer allocated by os_malloc().
  @anchor os_read_file_alloc

  The os_read_file_alloc() function reads a whole file into new buffer allocated by os_malloc().
  The allocated memory must be released by_os_free().

  @param   path Path to the file.
  @param   n_read Pointer to integer where to store number of bytes read.
  @param   flags OS_FILE_NULL_CHAR to terminate buffer with NULL character.
           OS_FILE_DEFAULT for default operation.

  @return  Pointer to file content in memory, or OS_NULL if the function failed.

****************************************************************************************************
*/
os_uchar *os_read_file_alloc(
    const os_char *path,
    os_memsz *n_read,
    os_int flags)
{
    osalFileStat filestat;
    osalStatus s;
    os_uchar *buf;

    *n_read = 0;

    /* Get file size
     */
    s = osal_filestat(path, &filestat);
    if (s) return OS_NULL;

    /* If we want to have terminating NULL character, reserve space for it.
     */
    if (flags & OS_FILE_NULL_CHAR) filestat.sz++;

    /* Allocate memory.
     */
    buf = (os_uchar*)os_malloc(filestat.sz, OS_NULL);
    if (buf == OS_NULL) return OS_NULL;

    /* Read the file.
     */
    s = os_read_file(path, buf, filestat.sz, n_read, flags);
    if (s || *n_read != filestat.sz)
    {
        os_free(buf, filestat.sz);
        return OS_NULL;
    }

    return buf;
}


/**
****************************************************************************************************

  @brief Write whole file from buffer.
  @anchor os_write_file

  The os_write_file() function writes a whole file from user given buffer.

  @param   path Path to the file.
  @param   buf Pointer to buffer,
  @param   n Number of bytes to write.
  @param   flags OS_FILE_NULL_CHAR to ignore n argument and get number of bytes to write by
           length of string in buffer.
           OS_FILE_DEFAULT for default operation.

  @return  OSAL_SUCCESS if all good, other values indicate an error.

****************************************************************************************************
*/
osalStatus os_write_file(
    const os_char *path,
    const os_uchar *buf,
    os_memsz n,
    os_int flags)
{
    osalStream f;
    osalStatus s;
    os_memsz n_written;

    /* If we want to have use os_strlen to determine number of bytes to write?
     */
    if (flags & OS_FILE_NULL_CHAR)
    {
        n = os_strlen((os_char*)buf) - 1;
    }

    /* Open file.
     */
    f = osal_file_open(path, OS_NULL, &s, OSAL_STREAM_WRITE);
    if (f == OS_NULL) return s;

    /* Write n bytes.
     */
    s = osal_file_write(f, buf, n, &n_written, OSAL_STREAM_DEFAULT);

    osal_file_close(f);
    return s;
}

#endif
