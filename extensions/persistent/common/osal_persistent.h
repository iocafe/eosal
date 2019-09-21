/**

  @file    persistent/common/osal_persistent.h
  @brief   Store persistent parameters.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Micro-controllers store persistent parameters in EEPROM or flash. These are typically static
  board configuration. Do not use flash to save any data which changes during normal operation,
  it will eventually destroy the flags (and thus the micro-controller).

  In PC simulation the data is saved in files in folder given as path.

  This file defines just function prototypes. Actual implementations differ by a lot depending
  wether persistant data is stored on EEPROM, micro-controller's flash or in file system.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_PERSISTENT_INCLUDED
#define OSAL_PERSISTENT_INCLUDED
#if OSAL_OSAL_PERSISTENT_SUPPORT

/* Parameters structure for os_persistent_initialze() function.
 */
typedef struct
{
    /* Path where to save persistent data during PC simulation.
     */
    os_char8 *path;
}
osPersistentParams;

/* Reserved persistent blocks. We need to have uniques persistant block number for each
   parameter block which can be saved.
 */
 typedef enum
 {
    OS_FIRST_CONFIG_PRM_BLK = 10,
    OS_LAST_CONFIG_PRM_BLK = OS_FIRST_CONFIG_PRM_BLK + 10 - 1,

    OS_FIRST_SOCKET_PRM_BLK = 10,
    OS_LAST_SOCKET_PRM_BLK = OS_FIRST_SOCKET_PRM_BLK + 10 - 1,

    OS_FIRST_TLS_PRM_BLK = 10,
    OS_LAST_TLS_PRM_BLK = OS_FIRST_SOCKET_TLS_BLK + 10 - 1,

    OS_FIRST_APPL_PRM_BLK = 1000,
 }
 oePersistentBlockNr;


/**
****************************************************************************************************

  @brief Initialize persistent storage access.

  The os_persistent_initialze() initializes the the persistent storage.
/* Parameters for os_persistent_initialze() function. Clear struture, set parameters and call
 * os_persistent_initialze():
   - osPersistentParams prm;
   - os_memclear(&prm, sizeof(prm));
   - os_persistent_initialze(&prm);

  @return  None.

****************************************************************************************************
*/
void os_persistent_initialze(
    osPersistentParams *prm);

os_memsz os_persistent_load_block(
    os_int block_nr,
    os_uchar *block,
    os_memsz block_sz);

osalStatus os_persistent_save_block(
    os_int block_nr,
    os_uchar *block,
    os_memsz block_sz);

#endif
#endif
