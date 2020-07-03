/**

  @file    eosal/extensions/program/linux/osal_program_device.h
  @brief   Write software to IO device.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    1.7.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#if OSAL_DEVICE_PROGRAMMING_SUPPORT

void osal_initialize_programming(
    void);

osalStatus osal_start_device_programming(
    void);

osalStatus osal_program_device(
    os_char *buf,
    os_memsz buf_sz);

osalStatus osal_finish_device_programming(
    os_uint checksum);

void osal_cancel_device_programming(
    void);

#else

/* Empty macros to allow build without programming support.
 */
#define osal_initialize_programming()
#define osal_start_device_programming() OSAL_STATUS_FAILED
#define osal_program_device(b,s) OSAL_STATUS_FAILED
#define osal_finish_device_programming(cs) OSAL_STATUS_FAILED
#define osal_cancel_device_programming()

#endif
