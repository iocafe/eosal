/**

  @file    persistent/common/osal_fsys_persistent.c
  @brief   Save persistent parameters on Linux/Windows.
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
#if OSAL_PERSISTENT_SUPPORT

#define OSAL_PERSISTENT_MAX_PATH 128

#ifdef OSAL_WIN32
static os_char rootpath[OSAL_PERSISTENT_MAX_PATH] = "c:\\tmp";
#else
static os_char rootpath[OSAL_PERSISTENT_MAX_PATH] = "/tmp";
#endif

static void os_persistent_make_path(
    os_int block_nr,
    os_char *path,
    os_memsz path_sz);


void os_persistent_initialze(
    osPersistentParams *prm)
{
    if (prm) {
        if (prm->path) {
            os_strncpy(rootpath, prm->path, sizeof(rootpath));
        }
    }
}


/* Load parameter structure identified by block number from persistant storage. Load all
   parameters when micro controller starts, not during normal operation. If data cannot
   be loaded, leaves the block as is. Returned value maxes at block_sz.
 */
os_memsz os_persistent_load(
    os_int block_nr,
    os_uchar *block,
    os_memsz block_sz)
{
    os_char path[OSAL_PERSISTENT_MAX_PATH];
    os_memsz n_read;

    os_persistent_make_path(block_nr, path, sizeof(path));
    os_read_file(path, block, block_sz, &n_read, 0);
    return n_read;
}

/* Save parameter structure to persistent storage and identigy it by block number.
 */
osalStatus os_persistent_save(
    os_int block_nr,
    os_uchar *block,
    os_memsz block_sz)
{
    os_char path[OSAL_PERSISTENT_MAX_PATH];

    os_persistent_make_path(block_nr, path, sizeof(path));
    return os_write_file(path, block, block_sz, 0);
}

static void os_persistent_make_path(
    os_int block_nr,
    os_char *path,
    os_memsz path_sz)
{
    os_char buf[16];

    osal_int_to_string(buf, sizeof(buf), block_nr);

    os_strncpy(path, rootpath, path_sz);
    os_strncat(path, "-", path_sz);
    os_strncat(path, buf, path_sz);
    os_strncat(path, ".dat", path_sz);
}

#endif
