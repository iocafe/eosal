/**

  @file    persistent/common/osal_persistent.h
  @brief   Store persistent parameters.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

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
#if OSAL_PERSISTENT_SUPPORT

/** Parameters structure for os_persistent_initialze() function.
 */
typedef struct
{
    /** Path where to save persistent data during PC simulation.
     */
    const os_char *path;

    /** In Windows/linux environment, the device name is used to change persistent file
        folder to keep settings for multiple IO device processess separate. Device name
        is without number.
     */
    const os_char *device_name;

    /** If set (nonzero) sets minimum required EEPROM size.
     */
    os_memsz min_eeprom_sz;
}
osPersistentParams;

#ifndef OS_PB_MAX_NETWORKS
#if OSAL_MICROCONTROLLER
    #define OS_PB_MAX_NETWORKS 4
#else
    #define OS_PB_MAX_NETWORKS 20
#endif
#endif

/** Reserved persistent parameter block numbers. We need to have and unique persistant block
    number for each parameter block which can be saved. The OS_PBNR_DEFAULTS
    is not actual memory block, but number reseved for marking default configuration.
    Do not change persistent block numbers used by iocom, these are also in Python
    code and documentation.
    Values OS_PBNR_CUST_A ... OS_PBNR_CUST_C can be used for storing any application specific
    persistent data.
 */
typedef enum
{
    OS_PBNR_UNKNOWN = 0,
    OS_PBNR_FLASH_PROGRAM = 1,
    OS_PBNR_CONFIG = 2,
    OS_PBNR_DEFAULTS = 3,
    OS_PBNR_SERVER_KEY = 4,
    OS_PBNR_SECRET = 5,
    OS_PBNR_SERVER_CERT = 6,
    OS_PBNR_CLIENT_CERT_CHAIN = 7,
    OS_PBNR_CUST_A = 8,
    OS_PBNR_CUST_B = 9,
    OS_PBNR_CUST_C = 10,
    OS_PBNR_ACCOUNTS_1 = 11,
    OS_PBNR_ACCOUNTS_2 = 12,
    OS_PBNR_ACCOUNTS_3 = 13,
    OS_PBNR_ACCOUNTS_4 = 14,
    OS_N_PBNR = (OS_PBNR_ACCOUNTS_1 + OS_PB_MAX_NETWORKS)
}
osPersistentBlockNr;

/* This structure is defined only for pointer type checking, it is never used.
 */
typedef struct
{
    os_int just_for_type_check;
}
osPersistentHandle;

/* Flags for persistent API functions.
 */
#define OSAL_PERSISTENT_DEFAULT 0
#define OSAL_PERSISTENT_READ 1
#define OSAL_PERSISTENT_WRITE 2
#define OSAL_PERSISTENT_SECRET 4


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
    const os_char **block,
    os_memsz *block_sz,
    os_int flags);

/* Open persistent block for reading or writing.
 */
osPersistentHandle *os_persistent_open(
    osPersistentBlockNr block_nr,
    os_memsz *block_sz,
    os_int flags);

/* Close persistent storage block. If this is flash program transfer, the boot bank
   may also be switched, etc, depending on flags.
 */
void os_persistent_close(
    osPersistentHandle *handle,
    os_int flags);

/* Load parameter structure identified by block number from persistant storage. Load all
   parameters when micro controller starts, not during normal operation. If data cannot
   be loaded, leaves the block as is. Returned value maxes at block_sz.
 */
os_memsz os_persistent_read(
    osPersistentHandle *handle,
    os_char *buf,
    os_memsz buf_sz);

/* Write data to persistent storage block.
 */
osalStatus os_persistent_write(
    osPersistentHandle *handle,
    os_char *buf,
    os_memsz buf_sz);

#endif
