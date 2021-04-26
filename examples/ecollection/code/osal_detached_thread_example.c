/**

  @file    eosal/examples/ecollection/code/osal_detached_thread_example.c
  @brief   Example code, create detached thread.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#include "osal_example_collection_main.h"

/** Parameter structure for creating new thread.
 */
typedef struct
{
    /** Thread event to "trig" thread to process some activity.
     */
    osalEvent thread_event;
}
MyThreadParameters;

/* Forward referred static functions.
 */
static void my_detached_thread(
    void *prm,
    osalEvent done);

/**
****************************************************************************************************
  Detached thread example entry point.
****************************************************************************************************
*/
void osal_detached_thread_example(void)
{
    MyThreadParameters myprm;
    os_int i;
    osal_console_write("detached thread example started\n");

    /* Clear parameter structure and create thread event. OSAL_EVENT_SET_AT_EXIT is set
     * that event is triggered when the process exit is requested by osal_exit(), etc.
     */
    os_memclear(&myprm, sizeof(myprm));
    myprm.thread_event = osal_event_create(OSAL_EVENT_SET_AT_EXIT);

    /* Start thread.
     */
    osal_thread_create(my_detached_thread, &myprm, OS_NULL, OSAL_THREAD_DETACHED);

    /* Just print some text.
     */
    for (i = 0; i < 10; i++) {
        osal_console_write("detached thread example running\n");
        os_sleep(1000);
    }

    osal_console_write("detached thread example exited\n");
}


/**
****************************************************************************************************

  @brief Thread function.

  The function is called to start executing code for newly created thread.

  @param   prm Pointer to parameters for new thread. In this example parameter pointer is valid as
           long as this thread runs.
  @param   done Event to set when thread which created this one may proceed.

****************************************************************************************************
*/
static void my_detached_thread(
    void *prm,
    osalEvent done)
{
    MyThreadParameters myprm;
    os_int i;

    /* Copy parameters into local stack. Do not use prm pointer after setting "done" event.
     */
    os_memcpy(&myprm, prm, sizeof(MyThreadParameters));

    /* Let thread which created this one to proceed.
     */
    osal_event_set(done);

    /* Print some text every 800 ms. React to process exit request immediately
     */
    for (i = 0; i<80; i++)
    {
        osal_event_wait(myprm.thread_event, 300);
        if (osal_stop()) {
            break;
        }
        osal_console_write("worker thread runs\n");
    }

    osal_event_delete(myprm.thread_event);
    osal_console_write("worker thread terminated\n");
}

