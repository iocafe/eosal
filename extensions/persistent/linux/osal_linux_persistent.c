/**

  @file    persistent/linux/osal_linux_persistent.c
  @brief   Save persistent parameters on Linux.
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
#if OSAL_PERSISTENT_SUPPORT

/* Default location where to keep persistent configuration data on Linux, Windows, etc.
   This can be overridden by compiler define OSAL_PERSISTENT_ROOT. Location is important
   since security secret (persistent block 5) may be kept kept here and file permissions
   must be set.
 */
#ifndef OSAL_PERSISTENT_ROOT
#define OSAL_PERSISTENT_ROOT "/coderoot/data"
#endif

/* STATIC VARIABLES HERE SHOULD BE MOVED TO GLOBAL STRUCTURE FOR WINDOWS DLLS */

static os_char rootpath[OSAL_PERSISTENT_MAX_PATH] = OSAL_PERSISTENT_ROOT;

static os_boolean initialized = OS_FALSE;

/* Persistent handle used with file system.
 */
typedef struct
{
    osalStream f;
}
osFsysPersistentHandle;

/* Maximum number of streamers when using static memory allocation.
 */
#if OSAL_DYNAMIC_MEMORY_ALLOCATION == 0
static osFsysPersistentHandle osal_persistent_handle[OS_N_PBNR];
#endif


/* Forward referred static functions.
 */
static void os_persistent_make_path(
    osPersistentBlockNr block_nr,
    os_char *path,
    os_memsz path_sz);


/**
****************************************************************************************************

  @brief Initialize persistent storage for use.
  @anchor os_persistent_initialize

  The os_persistent_initialize() function...

  @param   prm Pointer to parameters for persistent storage. For this implementation path
           member sets path to folder where to keep parameter files. Can be OS_NULL if not
           needed.

****************************************************************************************************
*/
void os_persistent_initialize(
    osPersistentParams *prm)
{
#if OSAL_DYNAMIC_MEMORY_ALLOCATION == 0
    os_memclear(osal_persistent_handle, sizeof(osal_persistent_handle));
#endif

    if (prm)
    {
        if (prm->path) {
            os_strncpy(rootpath, prm->path, sizeof(rootpath));
        }
        osal_mkdir(rootpath, 0);

        if (prm->subdirectory) {
            os_strncat(rootpath, "/", sizeof(rootpath));
            os_strncat(rootpath, prm->subdirectory, sizeof(rootpath));
            osal_mkdir(rootpath, 0);
        }
    }

    osal_mkdir(rootpath, 0);
    initialized = OS_TRUE;
}


/**
****************************************************************************************************

  @brief Release any resources allocated for the persistent storage.
  @anchor os_persistent_shutdown

  The os_persistent_shutdown() function is just place holder for future implementations.

****************************************************************************************************
*/
void os_persistent_shutdown(
    void)
{
}


/**
****************************************************************************************************

  @brief Get pointer to persistant data block within persistent storage.
  @anchor os_persistent_get_ptr

  If persistant storage is in micro-controller's flash, we can just get pointer to data block
  and data size. If persistant storage is on such media (file system, etc) that direct
  pointer is not available, the function returns OSAL_STATUS_NOT_SUPPORTED and function
  os_persistent_load() must be called to read the data to local RAM.

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   block Block pointer to set.
  @param   block_sz Pointer to integer where to store block size.
  @param   flags OSAL_PERSISTENT_SECRET flag enables accessing the secret. It must be only
           given in safe context.
  @return  OSAL_SUCCESS of successful. Value OSAL_STATUS_NOT_SUPPORTED indicates that
           pointer cannot be aquired on this platform and os_persistent_load() must be
           called instead. Other values indicate an error.

****************************************************************************************************
*/
osalStatus os_persistent_get_ptr(
    osPersistentBlockNr block_nr,
    const os_uchar **block,
    os_memsz *block_sz,
    os_int flags)
{
#if OSAL_RELAX_SECURITY == 0
    /* Reading or writing seacred block requires secret flag. When this function
       is called for data transfer, there is no secure flag and thus secret block
       cannot be accessed to break security.
     */
    if ((block_nr == OS_PBNR_SECRET || block_nr == OS_PBNR_SERVER_KEY || block_nr == OS_PBNR_ROOT_KEY) &&
        (flags & OSAL_PERSISTENT_SECRET) == 0)
    {
        return OSAL_STATUS_NOT_AUTOHORIZED;
    }
#endif

    return OSAL_STATUS_NOT_SUPPORTED;
}


/**
****************************************************************************************************

  @brief Open persistent block for reading or writing.
  @anchor os_persistent_open

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   block_sz Pointer to integer where to store block size when reading the persistent block.
           This is intended to know memory size to allocate before reading.
  @param   flags OSAL_PERSISTENT_READ or OSAL_PERSISTENT_WRITE. Flag OSAL_PERSISTENT_SECRET
           enables reading or writing the secret, and must be only given in safe context.

  @return  Persistant storage block handle, or OS_NULL if the function failed.

****************************************************************************************************
*/
osPersistentHandle *os_persistent_open(
    osPersistentBlockNr block_nr,
    os_memsz *block_sz,
    os_int flags)
{
    osFsysPersistentHandle *handle;
    os_char path[OSAL_PERSISTENT_MAX_PATH];
    osalStream f;
    osalFileStat filestat;
    osalStatus s;

#if OSAL_DYNAMIC_MEMORY_ALLOCATION == 0
    os_int count;
#endif

#if OSAL_RELAX_SECURITY == 0
    /* Reading or writing seacred block requires secret flag. When this function
       is called for data transfer, there is no secure flag and thus secret block
       cannot be accessed to break security.
     */
    if ((block_nr == OS_PBNR_SECRET || block_nr == OS_PBNR_SERVER_KEY || block_nr == OS_PBNR_ROOT_KEY) &&
        (flags & OSAL_PERSISTENT_SECRET) == 0)
    {
        return OS_NULL;
    }
#endif

    if (!initialized) os_persistent_initialize(OS_NULL);
    os_persistent_make_path(block_nr, path, sizeof(path));

    /* If we need to get block size?
     */
    if (block_sz)
    {
        if (flags & OSAL_PERSISTENT_READ)
        {
            s = osal_filestat(path, &filestat);
            if (s) return OS_NULL;
            *block_sz = filestat.sz;
        }
        else
        {
            *block_sz = 0;
        }
    }

    /* Open file.
     */
    f = osal_file_open(path, OS_NULL, OS_NULL,
        (flags & OSAL_PERSISTENT_READ) ? OSAL_STREAM_READ : OSAL_STREAM_WRITE);
    if (f == OS_NULL) goto getout;

    /* Allocate streamer structure, either dynamic or static.
     */
#if OSAL_DYNAMIC_MEMORY_ALLOCATION
    handle = (osFsysPersistentHandle*)os_malloc(sizeof(osFsysPersistentHandle), OS_NULL);
    if (handle == OS_NULL) {
        osal_file_close(f, OSAL_STREAM_DEFAULT);
        goto getout;
    }
#else
    handle = osal_persistent_handle;
    count = OS_N_PBNR;
    while (streamer->f != OS_NULL)
    {
        if (--count == 0)
        {
            handle = OS_NULL;
            goto getout;
        }
        handle++;
    }
#endif

    /* Initialize handle structure.
     */
    os_memclear(handle, sizeof(osFsysPersistentHandle));
    handle->f = f;

    return (osPersistentHandle*)handle;

getout:
    osal_debug_error_str("Reading persistent block from file failed: ", path);
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Close persistent storage block.
  @anchor os_persistent_close

  @param   handle Persistant storage handle.
  @param   flags OSAL_STREAM_DEFAULT (0) is all was written to persistant storage.
           OSAL_STREAM_INTERRUPT flag is set if transfer was interrupted.
  @return  If all is fine, the function returns OSAL_SUCCESS. Other values can be returned
           to indicate an error.

****************************************************************************************************
*/
osalStatus os_persistent_close(
    osPersistentHandle *handle,
    os_int flags)
{
    osFsysPersistentHandle *h;
    h = (osFsysPersistentHandle*)handle;

    if (h) if (h->f)
    {
        osal_file_close(h->f, OSAL_STREAM_DEFAULT);
        h->f = OS_NULL;
    }

#if OSAL_DYNAMIC_MEMORY_ALLOCATION
    os_free(handle, sizeof(osFsysPersistentHandle));
#endif
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Read data from persistent parameter block.
  @anchor os_persistent_read

  The os_persistent_read() function reads data from persistant storage block. Load all
  parameters when micro controller starts, not during normal operation.

  @param   handle Persistant storage handle.
  @param   buf Pointer to buffer where to load persistent data.
  @param   buf_sz Number of bytes to read to buffer.
  @return  Number of bytes read. Can be less than buf_sz if end of persistent block data has
           been reached. 0 is fine if at end. -1 Indicates an error.

****************************************************************************************************
*/
os_memsz os_persistent_read(
    osPersistentHandle *handle,
    os_char *buf,
    os_memsz buf_sz)
{
    os_memsz n_read;
    osalStatus s;
    osFsysPersistentHandle *h;
    h = (osFsysPersistentHandle*)handle;

    if (h) if (h->f)
    {
        s = osal_file_read(h->f, buf, buf_sz, &n_read, OSAL_STREAM_DEFAULT);
        if (s) goto getout;
        return n_read;
    }

getout:
    return -1;
}


/**
****************************************************************************************************

  @brief Append data to persistent block.
  @anchor os_persistent_write

  os_persistent_write() function appends buffer to data to write.

  @param   handle Persistant storage handle.
  @param   buf Buffer Pointer to data to write (append).
  @param   buf_sz block_sz Block size in bytes.

  @return  OSAL_SUCCESS indicates all fine, other return values indicate on error.

****************************************************************************************************
*/
osalStatus os_persistent_write(
    osPersistentHandle *handle,
    const os_char *buf,
    os_memsz buf_sz)
{
    os_memsz n_written;
    osalStatus s = OSAL_STATUS_FAILED;
    osFsysPersistentHandle *h;
    h = (osFsysPersistentHandle*)handle;

    if (h) if (h->f)
    {
        s = osal_file_write(h->f, buf, buf_sz, &n_written, OSAL_STREAM_DEFAULT);
        if (s) goto getout;
        return n_written == buf_sz ? OSAL_SUCCESS : OSAL_STATUS_DISC_FULL;
    }

getout:
    return s;
}


/**
****************************************************************************************************

  @brief Make path to parameter file.
  @anchor os_persistent_make_path

  The os_persistent_make_path() function generates path from root path given as argument
  and block number.

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   path Pointer to buffer where to store generated path.
  @param   path_sz Size of path buffer.

****************************************************************************************************
*/
static void os_persistent_make_path(
    osPersistentBlockNr block_nr,
    os_char *path,
    os_memsz path_sz)
{
    os_char buf[OSAL_NBUF_SZ];

    os_strncpy(path, rootpath, path_sz);
    os_strncat(path, "/persistent-", path_sz);
    osal_int_to_str(buf, sizeof(buf), block_nr);
    os_strncat(path, buf, path_sz);
    os_strncat(path, ".dat", path_sz);
}


/**
****************************************************************************************************

  @brief Wipe persistent data.

  The os_persistent_delete function deletes a persistent block or wipes out all persistent data.

  @param   block_nr Parameter block number.
  @param   flags Set OSAL_PERSISTENT_DELETE_ALL to delete all blocks or OSAL_PERSISTENT_DEFAULT
           to delete block number given as argument.
  @return  OSAL_SUCCESS if all good, other values indicate an error.

****************************************************************************************************
*/
osalStatus os_persistent_delete(
    osPersistentBlockNr block_nr,
    os_int flags)
{
    osalStatus s;
    if (flags & OSAL_PERSISTENT_DELETE_ALL) {
        s = osal_remove_recursive(rootpath, "*.dat", 0);
        if (s) {
            osal_debug_error_int("os_persistent_delete failed ", s);
        }
    }
    else {
        s = os_save_persistent(block_nr, OS_NULL, 0, OS_TRUE);
    }

    return s;
}

#endif
#endif
