/**

  @file    eosal/examples/persistent/code/persistent_example.c
  @brief   Test persistent storage.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"

#define TXT_SZ 32
typedef struct
{
    os_char txt1[TXT_SZ], txt2[TXT_SZ];
}
MyParams;

static MyParams prm_a, prm_b;
static os_timer t;
static os_int count;

/* If needed for the operating system, EOSAL_C_MAIN macro generates the actual C main() function.
 */
EOSAL_C_MAIN


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
    osal_console_write("persistent test started\n");
    os_get_timer(&t);
    count = 50;

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
    osPersistentHandle *h;
    os_memsz block_sz, sz;

    if (os_has_elapsed(&t, 3000))
    {
        os_memclear(&prm_a, sizeof(prm_a));
        h = os_persistent_open(OS_PBNR_CUST_A, &block_sz, OSAL_PERSISTENT_READ);
        if (h)
        {
            sz = sizeof(prm_a);
            if (block_sz == sz)
            {
                if (os_persistent_read(h, (void*)&prm_a, sz) == sz)
                {
                    osal_console_write("A = ");
                    osal_console_write(prm_a.txt1);
                    osal_console_write(", ");
                    osal_console_write(prm_a.txt2);
                    osal_console_write("\n");
                }
            }
            os_persistent_close(h, OSAL_PERSISTENT_DEFAULT);
        }

        os_memclear(&prm_b, sizeof(prm_b));
        h = os_persistent_open(OS_PBNR_CUST_B, &block_sz, OSAL_PERSISTENT_READ);
        if (h)
        {
            sz = sizeof(prm_b);
            if (block_sz == sz)
            {
                if (os_persistent_read(h, (void*)&prm_b, sz) == sz)
                {
                    osal_console_write("B = ");
                    osal_console_write(prm_b.txt1);
                    osal_console_write(", ");
                    osal_console_write(prm_b.txt2);
                    osal_console_write("\n");
                }
            }
            os_persistent_close(h, OSAL_PERSISTENT_DEFAULT);
        }


        osal_int_to_str(buf, sizeof(buf), count--);
        os_strncpy(prm_a.txt1, "txt a1: ", TXT_SZ);
        os_strncat(prm_a.txt1, buf, TXT_SZ);
        os_strncpy(prm_a.txt2, "txt a2: ", TXT_SZ);
        os_strncat(prm_a.txt2, buf, TXT_SZ);
        h = os_persistent_open(OS_PBNR_CUST_A, OS_NULL, OSAL_PERSISTENT_WRITE);
        if (h)
        {
            sz = sizeof(prm_a);
            if (os_persistent_write(h, (void*)&prm_a, sz) == OSAL_SUCCESS)
            {
                osal_console_write("A written ok\n");
            }
            os_persistent_close(h, OSAL_PERSISTENT_DEFAULT);
        }


        os_strncpy(prm_b.txt1, "txt b1: ", TXT_SZ);
        os_strncat(prm_b.txt1, buf, TXT_SZ);
        os_strncpy(prm_b.txt2, "txt b2: ", TXT_SZ);
        os_strncat(prm_b.txt2, buf, TXT_SZ);
        h = os_persistent_open(OS_PBNR_CUST_B, OS_NULL, OSAL_PERSISTENT_WRITE);
        if (h)
        {
            sz = sizeof(prm_b);
            if (os_persistent_write(h, (void*)&prm_b, sz) == OSAL_SUCCESS)
            {
                osal_console_write("B written ok\n");
            }
            os_persistent_close(h, OSAL_PERSISTENT_DEFAULT);
        }

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
