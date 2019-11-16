/**

  @file    persistent/shared/filesystem/osal_fsys_persistent.c
  @brief   Save persistent parameters on Linux/Windows.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.9.2019

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_PERSISTENT_SUPPORT

#define OSAL_PERSISTENT_MAX_PATH 128

#ifdef OSAL_WIN32
static os_char rootpath[OSAL_PERSISTENT_MAX_PATH] = "c:\\coderoot\\tmp";
#else
static os_char rootpath[OSAL_PERSISTENT_MAX_PATH] = "/coderoot/tmp";
#endif

static os_boolean initialized = OS_FALSE;

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

  @brief Load parameter block (usually structure) from persistent storage.
  @anchor os_persistent_load

  The os_persistent_load() function loads parameter structure identified by block number
  from the persistant storage. Load all parameters when micro controller starts, not during
  normal operation. If data cannot be loaded, the function leaves the block as is.

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   block Pointer to block (structure) to load.
  @param   block_sz Block size in bytes.
  @return  Number of bytes read. Zero if failed. Returned value maxes at block_sz.

****************************************************************************************************
*/
os_memsz os_persistent_load(
    osPersistentBlockNr block_nr,
    void *block,
    os_memsz block_sz)
{
    os_char path[OSAL_PERSISTENT_MAX_PATH];
    os_memsz n_read;

    if (!initialized) os_persistent_initialze(OS_NULL);

    os_persistent_make_path(block_nr, path, sizeof(path));
    os_read_file(path, block, block_sz, &n_read, 0);
    return n_read;
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
osalStatus os_persistent_save(
    osPersistentBlockNr block_nr,
    const void *block,
    os_memsz block_sz,
    os_boolean commit)
{
    os_char path[OSAL_PERSISTENT_MAX_PATH];

    if (!initialized) os_persistent_initialze(OS_NULL);

    os_persistent_make_path(block_nr, path, sizeof(path));
    return os_write_file(path, block, block_sz, OS_FILE_DEFAULT);
}


/**
****************************************************************************************************

  @brief Commit changes to persistent storage.
  @anchor os_persistent_commit

  The os_os_persistent_commit() function commit unsaved changed to persistent storage.

  @return  OSAL_SUCCESS indicates all fine, other return values indicate on error.

****************************************************************************************************
*/
osalStatus os_persistent_commit(
    void)
{
    return OSAL_SUCCESS;
}


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
    osal_int_to_string(buf, sizeof(buf), block_nr);
    os_strncat(path, buf, path_sz);
    os_strncat(path, ".dat", path_sz);
}

#endif
