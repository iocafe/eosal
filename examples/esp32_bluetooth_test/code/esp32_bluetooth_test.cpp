/**

  @file    esp32_bluetooth_test.c
  @brief   Test how bluetooth serial works on ESP32.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Just simple ESP32 bluetooth serial communication test. Doesn't run on any other hardware/OS.

****************************************************************************************************
*/
#include "eosalx.h"
#include <BluetoothSerial.h>

static BluetoothSerial SerialBT;

/* If needed for the operating system, EOSAL_C_MAIN macro generates the actual C main() function.
 */
EOSAL_C_MAIN

/**
****************************************************************************************************
  Initialize.
****************************************************************************************************
*/
osalStatus osal_main(
    os_int argc,
    os_char *argv[])
{
    SerialBT.begin("ESP32");

    /* When emulating micro-controller on PC, run loop. Just save context pointer on
       real micro-controller.
     */
    osal_simulated_loop(OS_NULL);
    return 0;
}


/**
****************************************************************************************************
  Run it.
****************************************************************************************************
*/
osalStatus osal_loop(
    void *app_context)
{
    SerialBT.println("Hello World");
    osal_sleep(1000);

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************
  Clean up.
****************************************************************************************************
*/
void osal_main_cleanup(
    void *app_context)
{
}


