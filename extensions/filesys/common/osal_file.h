/**

  @file    filesys/common/osal_file.h
  @brief   OSAL stream API for file IO.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  This header file contains function prototypes and definitions to implement OSAL stream API
  and for basic file IO.  OSAL stream API is abstraction which makes streams (including files)
  look similar to upper levels of code, regardless of operating system and implementation.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#if OSAL_FILESYS_SUPPORT

/** Stream interface structure for files.
 */
extern const osalStreamInterface osal_file_iface;

/** Define to get file interface pointer. The define is used so that this can
    be converted to function call.
 */
#define OSAL_FILE_IFACE &osal_file_iface

/* Offset between Windows FILETIME and UTC in microseconds since 1.1.1970.
 */
#define OSAL_WINDOWS_FILETIME_OFFSET 11644473600000000ULL


/**
****************************************************************************************************

  @name OSAL file Functions.

  These functions implement files as OSAL stream. These functions can either be called
  directly or through stream interface.

****************************************************************************************************
 */
/*@{*/

/* Open file.
 */
osalStream osal_file_open(
    const os_char *parameters,
    void *option,
    osalStatus *status,
    os_int flags);

/* Close file.
 */
void osal_file_close(
    osalStream stream,
    os_int flags);

/* Flush written data to file.
 */
osalStatus osal_file_flush(
    osalStream stream,
    os_int flags);

/* Write data to file.
 */
osalStatus osal_file_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags);

/* Read data from file.
 */
osalStatus osal_file_read(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags);

/*@}*/

#endif
