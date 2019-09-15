#include <Arduino.h>
#include <FreeRTOS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <eosalx.h>

/*
  simple_server_example
  Example to include simple server app to build it within Arduino IDE.
  The setup routine runs once when the device starts.
 */
void setup() 
{
    /*  Set up serial port for trace output.
     */
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Simple server starting (Arduino IDE mode)...");

    /* Initialize eosal library and start the very simple server.
     */
    osal_initialize(OSAL_INIT_DEFAULT);
    osal_main(0, 0);
}

/* The loop function is called repeatedly while the device runs.
 */
void loop()
{
    if (osal_loop(osal_application_context)) osal_reboot(0);
}

/* Include code for server (at the time of writing IP 192.168.1.201,
   TCP port 6368, but these are set in client app code)
 */
#include "/coderoot/eosal/examples/simple_server/code/simple_server_example.c"
