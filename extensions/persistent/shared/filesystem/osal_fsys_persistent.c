/**

  @file    persistent/shared/filesystem/osal_fsys_persistent.c
  @brief   Save persistent parameters on Linux/Windows.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_PERSISTENT_SUPPORT
#if OSAL_USE_SHARED_FSYS_PERSISTENT

/* Default location where to keep persistent configuration data on Linux, Windows, etc.
   This can be overridden by compiler define OSAL_PERSISTENT_ROOT. Location is important
   since security keys and passwords are kept here and file permissions must be set.
 */
#ifndef OSAL_PERSISTENT_ROOT
#ifdef OSAL_WIN32
  #define OSAL_PERSISTENT_ROOT "c:\\coderoot\\config"
#else
  #define OSAL_PERSISTENT_ROOT "/coderoot/config"
#endif
#endif

/* STATIC VARIABLES HERE SHOULD BE MOVED TO GLOBAL STRUCTURE FOR WINDOWS DLLS */

#define OSAL_PERSISTENT_MAX_PATH 128
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
  @anchor os_persistent_initialze

  The os_persistent_initialze() function...

  @param   prm Pointer to parameters for persistent storage. For this implementation path
           member sets path to folder where to keep parameter files. Can be OS_NULL if not
           needed.
  @return  None.

****************************************************************************************************
*/
void os_persistent_initialze(
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

        if (prm->device_name) {
            os_strncat(rootpath, "/", sizeof(rootpath));
            os_strncat(rootpath, prm->device_name, sizeof(rootpath));
            osal_mkdir(rootpath, 0);
        }
    }

    osal_mkdir(rootpath, 0);
    initialized = OS_TRUE;

    /* Initialize also secret for security. Mark persistent initialized before
       initializing the secret.
     */
#if OSAL_SECRET_SUPPORT
    osal_initialize_secret();
#endif
}


/**
****************************************************************************************************

  @brief Release any resources allocated for the persistent storage.
  @anchor os_persistent_shutdown

  The os_persistent_shutdown() function is just place holder for future implementations.
  @return  None.

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
  @param   block Pointer to block (structure) to load.
  @param   block_sz Block size in bytes.
  @param   flags OSAL_PERSISTENT_SECRET flag enables accessing the secret. It must be only
           given in safe context.
  @return  OSAL_SUCCESS of successfull. Value OSAL_STATUS_NOT_SUPPORTED indicates that
           pointer cannot be aquired on this platform and os_persistent_load() must be
           called instead. Other values indicate an error.

****************************************************************************************************
*/
osalStatus os_persistent_get_ptr(
    osPersistentBlockNr block_nr,
    const os_char **block,
    os_memsz *block_sz,
    os_int flags)
{
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

    /* Reading or writing seacred block requires secret flag. When this function
       is called for data transfer, there is no secure flag and thus secret block
       cannot be accessed to break security.
     */
    if (block_nr == OS_PBNR_SECRET && (flags & OSAL_PERSISTENT_SECRET) == 0)
    {
        return OS_NULL;
    }

    if (!initialized) os_persistent_initialze(OS_NULL);
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
    if (handle == OS_NULL) goto getout;
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
  @return  None.

****************************************************************************************************
*/
void os_persistent_close(
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
}


/**
****************************************************************************************************

  @brief Read data from persistent parameter block.
  @anchor os_persistent_read

  The os_persistent_read() function reads data from persistant storage block. Load all
  parameters when micro controller starts, not during normal operation.

  @param   handle Persistant storage handle.
  @param   block Pointer to buffer where to load persistent data.
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

  @brief Save parameter block to persistent storage.
  @anchor os_persistent_save

  The os_persistent_save() function saves a parameter structure to persistent storage and
  identifies it by block number.

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   block Pointer to block (structure) to save.
  @param   block_sz Block size in bytes.
  @return  OSAL_SUCCESS indicates all fine, other return values indicate on error.

****************************************************************************************************
*/
osalStatus os_persistent_write(
    osPersistentHandle *handle,
    os_char *buf,
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
  @return  None.

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


#endif
#endif
