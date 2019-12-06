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

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_PERSISTENT_INCLUDED
#define OSAL_PERSISTENT_INCLUDED
#if OSAL_PERSISTENT_SUPPORT


/** Parameters structure for os_persistent_initialze() function.
 */
typedef struct
{
    /** Path where to save persistent data during PC simulation.
     */
    os_char *path;

    /** If set (nonzero) sets minimum required EEPROM size.
     */
    os_memsz min_eeprom_sz;
}
osPersistentParams;

/** Reserved persistent parameter block numbers. We need to have and unique persistant block
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


typedef struct
{
    /* Maximum flash program size in bytes.
     */
    os_memsz flash_sz;

    /* Minumum number of bytes to write at once. The number of program bytes given to
       os_persistent_program() function must be divisible by this number.
     */
    os_memsz min_prog_block_sz;

    /* dual bank support ? do we need to know ? */
}
osProgrammingSpecs;

typedef enum
{
    OS_PROG_WRITE,
    OS_PROG_INTERRUPT,
    OS_PROG_COMPLETE
}
osProgCommand;


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

/* Release any resources.
 */
void os_persistent_shutdown(
    void);

/* If persistant storage is in micro-controller's flash, we can just get pointer to data block
   and data size. If persistant storage is on such media (file system, etc) that direct
   pointer is not available, the function returns OSAL_STATUS_NOT_SUPPORTED and function
   os_persistent_load() must be called to read the data to local RAM.
 */
osalStatus os_persistent_get_ptr(
    osPersistentBlockNr block_nr,
    void **block,
    os_memsz *block_sz);

/* Load parameter structure identified by block number from persistant storage. Load all
   parameters when micro controller starts, not during normal operation. If data cannot
   be loaded, leaves the block as is. Returned value maxes at block_sz.
 */
os_memsz os_persistent_load(
    osPersistentBlockNr block_nr,
    void *block,
    os_memsz block_sz);

/* Save parameter structure to persistent storage and identify it by block number.
 */
osalStatus os_persistent_save(
    osPersistentBlockNr block_nr,
    const void *block,
    os_memsz block_sz,
    os_boolean commit);

/* Commit changes by os_persistent_save() to persistent storage.
 */
osalStatus os_persistent_commit(
    void);

/* Get block size, dual bank support, etc. info for writing the
   micro controller code on this specific platform.
 */
osalStatus os_persistent_programming_specs(
    osProgrammingSpecs *specs);

/* Program micro controller's flash. Return value OSAL_STATUS_NOT_SUPPORTED
   indicates that flash cannot be programmed on this system.
 */
osalStatus os_persistent_program(
    osProgCommand cmd,
    const os_char *buf,
    os_int addr,
    os_memsz nbytes);

#endif
#endif
