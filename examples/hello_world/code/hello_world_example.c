/**

  @file    eosal/examples/hello_world/code/hello_world_example.c
  @brief   Just make sure it builds.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This example is just a test code to test if eosal builds on target platform and can write 
  to console.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"

static os_timer t;
static os_int count;



/**
****************************************************************************************************

  @brief Process entry point.

  The osal_main() function is OS independent entry point.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
osalStatus osal_main(
    os_int argc,
    os_char *argv[])
{
    osal_console_write("hello world starts\n");
    os_get_timer(&t);
    count = 10;

    /* When emulating micro-controller on PC, run loop. Does nothing on real micro-controller.
     */
    osal_simulated_loop(OS_NULL);
    return 0;
}


/**
****************************************************************************************************

  @brief Loop function to be called repeatedly.

  The osal_loop() function...

  @param   app_context Void pointer, reserved to pass context structure, etc.
  @return  The function returns OSAL_SUCCESS to continue running. Other return values are
           to be interprened as reboot on micro-controller or quit the program on PC computer.

****************************************************************************************************
*/
osalStatus osal_loop(
    void *app_context)
{
    os_char buf[OSAL_NBUF_SZ];

    /* Show count once per second.
     */
    if (os_elapsed(&t, 1000))
    {
        osal_int_to_string(buf, sizeof(buf), count--);
        osal_console_write("howdy ");
        osal_console_write(buf);
        osal_console_write("\n");
        os_get_timer(&t);
    }

    /* When count reaches zero: Reboot micro-controller or quit the program in PC computer.
     */
    return count >= 0 ? OSAL_SUCCESS : OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Finish with communication.

  The osal_main_cleanup() function closes the stream, then closes underlying stream library.
  Notice that the osal_stream_close() function does close does nothing if it is called with NULL
  argument.

  @param   app_context Void pointer, reserved to pass context structure, etc.
  @return  None.

****************************************************************************************************
*/
void osal_main_cleanup(
    void *app_context)
{
}
