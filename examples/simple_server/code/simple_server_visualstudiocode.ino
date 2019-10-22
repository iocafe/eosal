#include <Arduino.h>
#include <eosalx.h>

/*
  simple_server_example
  Example to build simple server app with Visual Studio Code + Platform IO + Arduino libraries. 
  The setup routine runs once when the device starts.
*/
void setup() 
{
    /* Set up serial port for trace output.
     */
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Simple server starting (Visual Studio Code + Platform IO + Arduino mode)...");

    /* Start the very simple server.
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
