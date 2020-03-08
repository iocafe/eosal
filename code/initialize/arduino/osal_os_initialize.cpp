/**

  @file    initialize/arduino/osal_os_initialize.c
  @brief   Operating system specific OSAL initialization.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Operating system specific OSAL initialization and shut down.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include <Arduino.h>
#include "eosal.h"

#ifndef OSAL_NVIC_RESET
  #ifdef STM32F4XX
    #define OSAL_NVIC_RESET
  #endif
#endif

#ifndef OSAL_NVIC_RESET
  #ifdef STM32L4XX
    #define OSAL_NVIC_RESET
  #endif
#endif

#ifdef OSAL_NVIC_RESET
  void NVIC_SystemReset(void);
#endif

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
#ifdef ESP_PLATFORM
    /* No watchdog timers.
     */
    disableLoopWDT();
    disableCore0WDT();
    disableCore1WDT();
#endif
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

#ifdef ESP_PLATFORM
    ESP.restart();
#endif

#ifdef OSAL_NVIC_RESET
    NVIC_SystemReset();
#endif
}
