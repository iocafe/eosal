/**

  @file    initialize/esp/osal_os_initialize.c
  @brief   Operating system specific OSAL initialization.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    12.3.2020

  Operating system specific OSAL initialization and shut down.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#ifdef OSAL_ESP32

#undef LOG_LOCAL_LEVEL
#define LOG_LOCAL_LEVEL ESP_LOG_WARN

#ifndef OSAL_ESPIDF_FRAMEWORK
#include "esp_log.h"
#include "esp_system.h"
#endif

/* Prototypes of forward referred static functions.
 */
static void osal_print_esp32_info(
    const os_char *label,
    os_long value);


/**
****************************************************************************************************

  @brief Operating system specific OSAL library initialization.
  @anchor osal_init_os_specific

  The osal_init_os_specific() function does operating system specific initialization
  OSAL library for use.

  @param  flags Bit fields. OSAL_INIT_DEFAULT (0) for normal initialization.
          OSAL_INIT_NO_LINUX_SIGNAL_INIT not to initialize linux signals.

  @return  None.

****************************************************************************************************
*/
void osal_init_os_specific(
    os_int flags)
{
    /* Set esp-idf event logging levels.
     */
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set("wifi", ESP_LOG_INFO);
    esp_log_level_set("dhcpc", ESP_LOG_INFO);

    /* No watchdog timers.
     */
    disableLoopWDT();
    disableCore0WDT();
    disableCore1WDT();

    /* Print amount of heap and PS ram
     */
    osal_print_esp32_info("\nTotal heap: ", (os_long)ESP.getHeapSize());
    osal_print_esp32_info("Free heap: ", (os_long)ESP.getFreeHeap());
    osal_print_esp32_info("Total PSRAM:  ", (os_long)ESP.getPsramSize());
    osal_print_esp32_info("Free PSRAM: ", (os_long)ESP.getFreePsram());
}


/**
****************************************************************************************************

  @brief Print ESP32 information to console.
  @anchor osal_print_esp32_info

  The osal_print_esp32_info() function just prints label and value to console.

  @param  label Label text
  @param  value Value to print.
  @return  None.

****************************************************************************************************
*/
static void osal_print_esp32_info(
    const os_char *label,
    os_long value)
{
    os_char nbuf[OSAL_NBUF_SZ];

    osal_console_write(label);
    osal_int_to_str(nbuf, sizeof(nbuf), value);
    osal_console_write(nbuf);
    osal_console_write("\n");
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
#if OSAL_INTERRUPT_LIST_SUPPORT
    osal_control_interrupts(OS_FALSE);
#endif

    os_sleep(200);

    esp_restart();
}

#endif
