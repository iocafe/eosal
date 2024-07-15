/**

  @file    initialize/esp32/osal_esp32_initialize.c
  @brief   Operating system specific initialization.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    3.9.2021

  Operating system specific initialization for ESP-IDF sets log message levels, disables watchdog
  timers and prints some HW information. This file also implements osal_reboot() function to 
  restart the microcontroller.

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

#include "esp_log.h"
#include "esp_system.h"
#include "spi_flash_mmap.h"
#include "esp_heap_caps.h"
#include "esp_chip_info.h"
#include "bootloader_random.h"

/* Prototypes of forward referred static functions.
 */
static void osal_print_esp32_info(
    const os_char *label,
    os_long value);


/**
****************************************************************************************************

  @brief Operating system specific OSAL library initialization.
  @anchor osal_init_os_specific

  The osal_init_os_specific() function for ESP32F sets log message levels, disables watchdog
  timers and prints some HW information. 
  
  @param  flags Bit fields. OSAL_INIT_DEFAULT (0) for normal initialization.
          OSAL_INIT_NO_LINUX_SIGNAL_INIT not to initialize linux signals.

****************************************************************************************************
*/
void osal_init_os_specific(
    os_int flags)
{
    esp_chip_info_t chip_info;

    /* Set esp-idf event logging levels.
     */
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set("wifi", ESP_LOG_INFO);
    esp_log_level_set("dhcpc", ESP_LOG_INFO);

    /* No watchdog timers.
     */
    rtc_wdt_protect_off();
    rtc_wdt_disable();

    /* This was the original esp32-arduino only implementation.
    disableLoopWDT();
    disableCore0WDT();
    disableCore1WDT();
     */

    /* Print some generic system information.
     */
    esp_chip_info(&chip_info);
    osal_print_esp32_info("\nNro cores:   ", chip_info.cores);
    osal_print_esp32_info("Silicon rev: ", chip_info.revision);
    osal_console_write   ("WiFi:        ");
    if (chip_info.features & CHIP_FEATURE_BT) {
        osal_console_write("/BT");
    }
    if (chip_info.features & CHIP_FEATURE_BLE) {
        osal_console_write("/BLE");
    }
    osal_console_write("\n");
    osal_print_esp32_info("Silicon rev: ", chip_info.revision);
    osal_print_esp32_info((chip_info.features & CHIP_FEATURE_EMB_FLASH) 
      ? "Flash emb:"    : "Flash ext:   ", spi_flash_get_chip_size());

    /* Print amount of heap and PS ram
     */
#ifndef OSAL_ESPIDF_FRAMEWORK
    osal_print_esp32_info("Total heap:  ", (os_long)ESP.getHeapSize());
    osal_print_esp32_info("Free heap:   ", (os_long)ESP.getFreeHeap());
    osal_print_esp32_info("Total PSRAM: ", (os_long)ESP.getPsramSize());
    osal_print_esp32_info("Free PSRAM:  ", (os_long)ESP.getFreePsram());
#else
    osal_print_esp32_info("Total heap:  ", (os_long)heap_caps_get_total_size(MALLOC_CAP_8BIT));
    osal_print_esp32_info("Free heap:   ", (os_long)heap_caps_get_free_size(MALLOC_CAP_8BIT));
    osal_print_esp32_info("Total PSRAM: ", (os_long)heap_caps_get_total_size(MALLOC_CAP_SPIRAM)); 
    osal_print_esp32_info("Free PSRAM:  ", (os_long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM)); 
#endif    
    osal_console_write("\n");
}


/**
****************************************************************************************************

  @brief Print ESP32 information to console.
  @anchor osal_print_esp32_info

  The osal_print_esp32_info() is a helper function to prints a label and value to console.

  @param  label Label text
  @param  value Value to print.

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

  @brief Shut down operating system.
  @anchor osal_shutdown

  Shutdown is not needed for ESP32.

****************************************************************************************************
*/
void osal_shutdown_os_specific(
    void)
{
}


/**
****************************************************************************************************

  @brief Restart the microcontroller.
  @anchor osal_reboot

  @param   flags Reserved for future, set 0 for now.

****************************************************************************************************
*/
void osal_reboot(
    os_int flags)
{
#if OSAL_INTERRUPT_LIST_SUPPORT
    osal_control_interrupts(OS_FALSE);
#endif

    osal_sleep(200);
    esp_restart();
}


#ifdef OSAL_DUMMY_ESP32_APP_MAIN
/**
****************************************************************************************************

  @brief Dummy main function to allow testing eosal ESP32 build configuration.
  @anchor app_main

  Dummy app_main() function allows compiling eosal separately as application (which does
  nothing) and allows to test ESPIDF/platformio build.

****************************************************************************************************
*/
#ifdef OSAL_ESPIDF_FRAMEWORK
extern "C" void app_main(void)
{
    osal_initialize(OSAL_INIT_DEFAULT);
}

#else
void setup()
{
    osal_initialize(OSAL_INIT_DEFAULT);
}
void loop()
{
}
#endif
#endif

#endif

