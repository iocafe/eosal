#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <eosalx.h>

/*
  simple_client_visualstudiocode.ino
  Example to build simple client app with Visual Studio Code + Platform IO + Arduino libraries. 
  The setup routine runs once when the device starts.
 */
void setup() 
{
    /* Set up serial port for trace output.
     */
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Simple client starting (Arduino IDE mode)...");

    /* Initialize eosal library and start the simple client.
     */
    osal_initialize(OSAL_INIT_DEFAULT);
    osal_main(0, 0);
}

/* The loop function is called repeatedly while the device runs.
 */
void loop() 
{
    /* Start the included application.
     */
    if (osal_loop(OS_NULL)) osal_reboot(0);
}

