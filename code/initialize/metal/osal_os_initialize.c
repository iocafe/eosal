/**

  @file    initialize/metal/osal_os_initialize.c
  @brief   Bare metal system specific OSAL initialization.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.12.2017

  Operating system specific OSAL initialization and shut down.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"

void NVIC_SystemReset(void);

/**
****************************************************************************************************

  @brief Operating system specific OSAL library initialization.
  @anchor osal_init_os_specific

  The osal_init_os_specific() function does operating system specific initialization
  OSAL library for use.

  @param  flags Bit fields. OSAL_INIT_DEFAULT (0) for normal initalization.
          OSAL_INIT_NO_LINUX_SIGNAL_INIT not to initialize linux signals.

  @return  None.

****************************************************************************************************
*/
void osal_init_os_specific(
    os_int flags)
{
}


/**
****************************************************************************************************

  @brief Shut down operating system specific part of the OSAL library.
  @anchor osal_shutdown

  The osal_shutdown_os_specific() function...

  @return  None.

****************************************************************************************************
*/
void osal_shutdown_os_specific(
    void)
{
}


/**
****************************************************************************************************

  @brief Reboot the computer.
  @anchor osal_reboot

  The osal_reboot() function...

  @param   flags Reserved for future, set 0 for now.
  @return  None.

****************************************************************************************************
*/
void osal_reboot(
    os_int flags)
{
    os_sleep(200);
    NVIC_SystemReset();
}
