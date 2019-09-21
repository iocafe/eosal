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

/* Reserved persistent parameter block numbers. We need to have and unique persistant block
   number for each parameter block which can be saved.
 */
 typedef enum
 {
    OS_PBNR_IO_DEVICE = 0,
    OS_PBNR_NIC_1 = 1,
    OS_PBNR_NIC_2 = 2,
    OS_PBNR_NIC_3_= 3,
    OS_PBNR_CON_1 = 4,
    OS_PBNR_CON_2 = 5,
    OS_PBNR_CON_3 = 6,
    OS_PBNR_SRV_1 = 7,
    OS_PBNR_SRV_2 = 8,
    OS_PBNR_UDP_1 = 9,
    OS_PBNR_APP_1 = 10,
    OS_PBNR_APP_2 = 11,
    OS_PBNR_APP_3 = 12,
    OS_PBNR_RESERVE_1 = 13,
    OS_PBNR_RESERVE_2 = 14,
    OS_PBNR_RESERVE_3 = 15,

    OS_N_PBNR = 16
 }
 osPersistentBlockNr;


/**
****************************************************************************************************

  @brief Initialize persistent storage access.

  @return  None.

****************************************************************************************************
*/

/* Initialize persistent storage for use.
 */
void os_persistent_initialze(
    osPersistentParams *prm);

/* Load parameter structure identified by block number from persistant storage. Load all
   parameters when micro controller starts, not during normal operation. If data cannot
   be loaded, leaves the block as is. Returned value maxes at block_sz.
 */
os_memsz os_persistent_load(
    osPersistentBlockNr block_nr,
    os_uchar *block,
    os_memsz block_sz);

/* Save parameter structure to persistent storage and identify it by block number.
 */
osalStatus os_persistent_save(
    osPersistentBlockNr block_nr,
    os_uchar *block,
    os_memsz block_sz);

#endif
#endif
