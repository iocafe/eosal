/**

  @file    persistent/shared/filesystem/osal_fsys_persistent.c
  @brief   Save persistent parameters on Linux/Windows.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.9.2019

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_PERSISTENT_SUPPORT
#if OSAL_USE_SHARED_FSYS_PERSISTENT

#define OSAL_PERSISTENT_MAX_PATH 128

#ifdef OSAL_WIN32
static os_char rootpath[OSAL_PERSISTENT_MAX_PATH] = "c:\\coderoot\\tmp";
#else
static os_char rootpath[OSAL_PERSISTENT_MAX_PATH] = "/coderoot/tmp";
#endif

static os_boolean initialized = OS_FALSE;

/* Maximum number of streamers when using static memory allocation.
 */
#if OSAL_DYNAMIC_MEMORY_ALLOCATION == 0
static osPersistentHandle osal_persistent_handle[OSAL_MAX_PERSISTENT_HANDLES];
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

    if (prm) {
        if (prm->path) {
            os_strncpy(rootpath, prm->path, sizeof(rootpath));
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
  @return  OSAL_SUCCESS of successfull. Value OSAL_STATUS_NOT_SUPPORTED indicates that
           pointer cannot be aquired on this platform and os_persistent_load() must be
           called instead. Other values indicate an error.

****************************************************************************************************
*/
osalStatus os_persistent_get_ptr(
    osPersistentBlockNr block_nr,
    os_char **block,
    os_memsz *block_sz)
{
    return OSAL_STATUS_NOT_SUPPORTED;
}


/**
****************************************************************************************************

  @brief Open persistent block for reading or writing.
  @anchor os_persistent_open

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   flags OSAL_STREAM_READ, OSAL_STREAM_WRITE

  @return  Persistant storage block handle, or OS_NULL if the function failed.

****************************************************************************************************
*/
osPersistentHandle *os_persistent_open(
    osPersistentBlockNr block_nr,
    os_int flags)
{
    osPersistentHandle *handle;
    os_char path[OSAL_PERSISTENT_MAX_PATH];
    osalStream f;

#if OSAL_DYNAMIC_MEMORY_ALLOCATION == 0
    os_int count;
#endif

    if (!initialized) os_persistent_initialze(OS_NULL);
    os_persistent_make_path(block_nr, path, sizeof(path));

    /* Open file.
     */
    f = osal_file_open(path, OS_NULL, OS_NULL, flags);
    if (f == OS_NULL) goto getout;

    /* Allocate streamer structure, either dynamic or static.
     */
#if OSAL_DYNAMIC_MEMORY_ALLOCATION
    handle = (osPersistentHandle*)os_malloc(sizeof(osPersistentHandle), OS_NULL);
    if (handle == OS_NULL) goto getout;
#else
    handle = osal_persistent_handle;
    count = OSAL_MAX_PERSISTENT_HANDLES;
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
    os_memclear(handle, sizeof(osPersistentHandle));
    handle->f = f;
    handle->flags = flags;
    return handle;

getout:
    osal_debug_error("Opening persistent block failed");
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Close persistent storage block.
  @anchor os_persistent_close

  @param   handle Persistant storage handle.
  @param   flags ?
  @return  None.

****************************************************************************************************
*/
void os_persistent_close(
    osPersistentHandle *handle,
    os_int flags)
{
    if (handle) if (handle->f)
    {
        osal_file_close(handle->f, OSAL_STREAM_DEFAULT);
        handle->f = OS_NULL;
    }

#if OSAL_DYNAMIC_MEMORY_ALLOCATION
    os_free(handle, sizeof(osPersistentHandle));
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
           been reached.

****************************************************************************************************
*/
os_memsz os_persistent_read(
    osPersistentHandle *handle,
    os_char *buf,
    os_memsz buf_sz)
{
    os_memsz n_read;
    osalStatus s;

    if (handle) if (handle->f)
    {
        s = osal_file_read(handle->f, buf, buf_sz, &n_read, OSAL_STREAM_DEFAULT);
        if (s) goto getout;
        return n_read;
    }

getout:
    return 0;
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

    if (handle) if (handle->f)
    {
        s = osal_file_write(handle->f, buf, buf_sz, &n_written, OSAL_STREAM_DEFAULT);
        if (s) goto getout;
        return n_written == buf_sz ? OSAL_SUCCESS : OSAL_STATUS_DISC_FULL;
    }

getout:
    return s;
}


/**
****************************************************************************************************

  @brief Commit changes to persistent storage.
  @anchor os_persistent_commit

  The os_os_persistent_commit() function commit unsaved changed to persistent storage.

  @return  OSAL_SUCCESS indicates all fine, other return values indicate on error.

****************************************************************************************************
*/
/*
osalStatus os_persistent_commit(
    void)
{
    return OSAL_SUCCESS;
} */


/**
****************************************************************************************************

  @brief Make path to parameter file.
  @anchor os_persistent_make_path

  The os_persistent_make_path() function generates path from root path given as argument
  and block number.

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   block Pointer to block (structure) to save.
  @param   block_sz Block size in bytes.
  @return  OSAL_SUCCESS indicates all fine, other return values indicate on error.

****************************************************************************************************
*/
static void os_persistent_make_path(
    osPersistentBlockNr block_nr,
    os_char *path,
    os_memsz path_sz)
{
    os_char buf[OSAL_NBUF_SZ];

    os_strncpy(path, rootpath, path_sz);
    if (path[os_strlen(path)-1] != '/')
    {
        os_strncat(path, "/", path_sz);
    }

    os_strncat(path, "iodevprm-", path_sz);
    os_strncat(path, "-", path_sz);
    osal_int_to_str(buf, sizeof(buf), block_nr);
    os_strncat(path, buf, path_sz);
    os_strncat(path, ".dat", path_sz);
}


/**
****************************************************************************************************

  @brief Get platform info for writing a flash program.
  @anchor os_persistent_programming_specs

  Get block size, etc. info for writing the micro controller code on this specific platform.

  @param   specs Pointer to programming specs structure to fill in. Set up only if return
           value is OSAL_SUCCESS.
  @return  OSAL_SUCCESS if all is good. Return value OSAL_STATUS_NOT_SUPPORTED indicates
           that flash cannot be programmed on this system.

****************************************************************************************************
*/
/* osalStatus os_persistent_programming_specs(
    osProgrammingSpecs *specs)
{
    return OSAL_STATUS_NOT_SUPPORTED;
} */



#endif
#endif
