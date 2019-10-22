#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include <BluetoothSerial.h>
#include <eosalx.h>

/*
  esp32_bluetooth_test.ino
  Include a ESP32 serial bluetooth test app to build it within Arduino IDE.
 */
 
/* The setup routine runs once when you press reset.
 */
void setup() 
{
    /* Set up serial port for trace output.
     */
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("Arduino IO board starting...");

   /* Initialize the eosal library. It is not really needed for the test, but
      included just to test for build issues.
    */
    osal_initialize(OSAL_INIT_DEFAULT);
    osal_main(0, 0);
}

/* The loop routine runs over and over again forever.
 */
void loop() 
{
    /* Forward loop call to osal_loop(). Reboot if osal_loop returns "no success".
     */
    if (osal_loop(osal_application_context)) osal_reboot(0);
}
