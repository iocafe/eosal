# 1 "/tmp/tmpX3S8Nv"
#include <Arduino.h>
# 1 "/coderoot/eosal/examples/simple_client/code/simple_client_visualstudiocode.ino"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <eosalx.h>
void setup();
void loop();
#line 10 "/coderoot/eosal/examples/simple_client/code/simple_client_visualstudiocode.ino"
void setup()
{


    Serial.begin(115200);
    while (!Serial);
    Serial.println("Simple client starting (Arduino IDE mode)...");



    osal_initialize(OSAL_INIT_DEFAULT);
    osal_main(0, 0);
}



void loop()
{


    if (osal_loop(OS_NULL)) osal_reboot(0);
}