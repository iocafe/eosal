/**

  @file    persistent/common/osal_persistent.h
  @brief   Store persistent parameters.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.9.2019

  Micro-controllers store persistent board configuration parameters in EEPROM or flash. On Windows/Linux
  these parameters are usually saved in files within OS file system.

  The osal_persistent.h which declares how the persistent storage access functions look like
  and defines uses for the persistent parameter block numbers. Actual implementations differ
  by a lot depending wether persistant data is stored on EEPROM, micro-controller's flash
  or in file system, used hardware, ect.

  There are various implementations: Flash and EEPROM hardware use is typically microcontroller
  specific, and common library functionality can rarely be used. On Arduino there is valuable
  effort to make this portable, but even with Arduino we cannot necessarily use the portable
  code: To be specific we cannot use Arduino EEPROM emulation relying on flash with secure
  flash program updates over TLS.
  On raw metal or custom SPI connected EEPROMS we always end up with micro-controller or board
  specific code. On Windows or Linux this is usually simple, we can just save board configuration
  in file system. Except if we have read only file system on linux.

  The actual implementations are in subdirectories for different platforms, boards and setups.
  The subdirectories named after platform, like "arduino", "linux" or "windows" contain either
  the persistent storage implementations for the platform or include C code from "shared"
  directory, when a generic implementation can be used. The "metal" subdirectory is used
  typically with microcontroller without any operating system.

  Warning: Do not use micro-controller flash to save any data which changes during normal
  run time operation, it will eventually burn out the flash (death of the micro controller).

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_PERSISTENT_INCLUDED
#define OSAL_PERSISTENT_INCLUDED
#if OSAL_PERSISTENT_SUPPORT


/* Parameters structure for os_persistent_initialze() function.
 */
typedef struct
{
    /* Path where to save persistent data during PC simulation.
     */
    os_char *path;
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
    OS_LAST_TLS_PRM_BLK = OS_FIRST_TLS_PRM_BLK + 10 - 1,

    OS_FIRST_APP_PRM_BLK = 101
 }
 oePersistentBlockNr;


/**
****************************************************************************************************

  @brief Initialize persistent storage access.

  @return  None.

****************************************************************************************************
*/
void os_persistent_initialze(
    osPersistentParams *prm);

/* Load parameter structure identified by block number from persistant storage. Load all
   parameters when micro controller starts, not during normal operation. If data cannot
   be loaded, leaves the block as is. Returned value maxes at block_sz.
 */
os_memsz os_persistent_load(
    os_int block_nr,
    os_uchar *block,
    os_memsz block_sz);

/* Save parameter structure to persistent storage and identigy it by block number.
 */
osalStatus os_persistent_save(
    os_int block_nr,
    os_uchar *block,
    os_memsz block_sz);

#endif
#endif
