#include <Arduino.h>
#include <eosalx.h>


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
//  Serial.begin(9600);
   osal_initialize(OSAL_INIT_DEFAULT);
}

void loop()
{
    Serial.println("Hello world!");
    delay(1000);  
    osal_main(0,0);
}


/* Include code for example client application (at the time of writing connects socket 
   to IP 192.168.1.220, TCP port 6001)
 */
#include "/coderoot/eosal/examples/simple_socket_client/code/simple_socket_client_example.c"