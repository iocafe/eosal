/**

  @file    eosal/extensions/program/linux/osal_linux_program_device.c
  @brief   Write IO device program to executables.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    1.7.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_DEVICE_PROGRAMMING_SUPPORT

void osal_start_device_programming(void)
{
    osal_debug_error("HERE start prog");
}

void osal_program_device(
    os_char *buf,
    os_memsz buf_sz)
{
    osal_debug_error("HERE prog");
}

void osal_finish_device_programming(
    os_uint checksum)
{
    osal_debug_error("HERE finish prog");

//        osal_reboot(0);

}

void osal_cancel_device_programming(void)
{
    osal_debug_error("HERE cancel prog");
}

#endif
