/**

  @file    filesys/common/osal_dir.h
  @brief   Directory related functions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Functions for listing, creating and removing a directory.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_DIR_H_
#define OSAL_DIR_H_
#include "eosalx.h"

#if OSAL_FILESYS_SUPPORT

/** Directory list item.
 */
typedef struct osalDirListItem
{
    /** File name without path.
     */
    os_char *name;

    /** File size in bytes.
     */
    os_long sz;

    /** File modification time.
     */
    os_int64 tstamp;

    /** OS_TRUE if item is directory, OS_FALSE if file
     */
    os_boolean isdir;

    /** Pointer to next directory list item, OS_NULL if none.
     */
    struct osalDirListItem *next;
}
osalDirListItem;

/* Flags for osal_dir function.
 */
#define OSAL_DIR_DEFAULT 0
#define OSAL_DIR_FILESTAT 1

/** 
****************************************************************************************************

  @name OSAL file Functions.

  These functions implement files as OSAL stream. These functions can either be called
  directly or through stream interface.

****************************************************************************************************
 */
/*@{*/

/* List directory. Allocates directory list list.
 */
osalStatus osal_dir(
    const os_char *path,
    const os_char *wildcard,
    osalDirListItem **list,
	os_int flags);

/* Release directory list from memory.
 */
void osal_free_dirlist(
    osalDirListItem *list);

/* Create directory.
 */
osalStatus osal_mkdir(
    const os_char *path,
    os_int flags);

/* Remove directory.
 */
osalStatus osal_rmdir(
    const os_char *path,
	os_int flags);

/*@}*/

#endif
#endif
