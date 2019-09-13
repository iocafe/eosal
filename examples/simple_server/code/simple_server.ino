// #include <Arduino.h>
// #include <FreeRTOS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <eosal.h>
#include <eosalx.h>

/*
  simple_server_example
  Example to include simple server app to build it within Arduino IDE. 
 */
 
#define N_LEDS 3
//const int leds[N_LEDS] = {PB0, PB14, PB7};
 const int leds[N_LEDS] = {2,3, 4};
os_timer ledtimer;

static void toggle_leds(void)
{
    static int lednr = 0;
    if (os_elapsed(&ledtimer, 30))
    {
      digitalWrite(leds[lednr], LOW);
      if (++lednr >= N_LEDS) lednr = 0;
      digitalWrite(leds[lednr], HIGH);
      os_get_timer(&ledtimer);
    }
}


// Include code for eacho server (at the time of writing IP 192.168.1.201, TCP port 6001)
// #include "/coderoot/eosal/examples/simple_server/code/simple_server_example.c"

void setup() 
{
    int lednr;

    // initialize the digital pin as an output.
/*    for (lednr = 0; lednr < N_LEDS; lednr++)
    {
        pinMode(leds[lednr], OUTPUT);
    } */

    // Set up serial port for trace output.
    Serial.begin(115200);
    while (!Serial)
    {
        //toggle_leds();
        os_sleep(100);
    }
    Serial.println("Simple server starting (Arduino mode)...");

    // Start the very simple echo socket server.
    osal_initialize(OSAL_INIT_DEFAULT);
    osal_main(0, 0);
}

void loop()
{
    // toggle_leds();
    osal_loop(OS_NULL);
}
