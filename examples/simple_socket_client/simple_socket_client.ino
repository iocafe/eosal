#include <eosalx.h>

/*
  simple_socket_client.ino
  Include a simple client app to build it within Arduino IDE. 
 */
 
/* the setup routine runs once when you press reset.
 */
void setup() 
{
    /* Set up serial port for trace output.
     */
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("Arduino starting...");

   /* Initialize the eosal library.
    */
    osal_initialize(OSAL_INIT_DEFAULT);
}

/* the loop routine runs over and over again forever.
 */
void loop() 
{
    /* Start the included application.
     */
    osal_main(0,0);
}

/* Include code for example client application (at the time of writing connects socket 
   to IP 192.168.1.220, TCP port 6001)
 */
#include "/coderoot/eosal/examples/simple_socket_client/code/simple_socket_client_example.c"
