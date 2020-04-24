#include <eosalx.h>

/*
  simple_client.ino
  Include a simple client app to build it within Arduino IDE.
  The setup routine runs once when the device starts.
 */
void setup()
{
    /* Initialize eosal library and start the simple client.
     */
    osal_initialize(OSAL_INIT_DEFAULT);
    osal_main(0, 0);
}

/* The loop function is called repeatedly while the device runs.
 */
void loop()
{
    /* Forward loop call to osal_loop(). Reboot if osal_loop returns "no success".
     */
    if (osal_loop(osal_application_context)) osal_reboot(0);
}

/* Include code for client application (at the time of writing connects socket
   to IP 192.168.1.220, TCP port 6368, but these are set in client app code)
 */
#include "/coderoot/eosal/examples/simple_client/code/simple_client_example.c"
