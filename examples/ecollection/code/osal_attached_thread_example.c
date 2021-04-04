/**

  @file    eosal/examples/ecollection/code/osal_attached_thread_example.c
  @brief   Example code, create attached thread.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "osal_example_collection_main.h"
#include <stdio.h>

/** Parameter structure for the new thread.
 */
typedef struct
{
    /** Thread event to "trig" thread that there is some activity.
     */
    osalEvent thread_event;

    /** Flag to exit_request the thread.
     */
    os_boolean exit_request;
}
MyThreadParameters;


/* Forward referred static functions.
 */
static void my_attached_thread(
    void *prm,
    osalEvent done);


/**
****************************************************************************************************

  @brief Process entry point.

  The osal_main() function is OS independent entry point.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

****************************************************************************************************
*/
osalStatus osal_attached_thread_example(
    os_int argc,
    os_char *argv[])
{
    MyThreadParameters myprm;
    osalThread *handle;

    OSAL_UNUSED(argc);
    OSAL_UNUSED(argv);

    /* Clear parameter structure and create thread event.
     */
    os_memclear(&myprm, sizeof(myprm));
    myprm.thread_event = osal_event_create();

    /* Start thread.
     */
    handle = osal_thread_create(my_attached_thread, &myprm, OS_NULL, OSAL_THREAD_ATTACHED);

    /* Do the work, not much here.
     */
    os_sleep(2000);
    osal_console_write("parent thread runs\n");
    osal_event_set(myprm.thread_event);
    os_sleep(1000);

    /* Request the worker thread to exit and wait until done.
     */
    osal_console_write("requesting child thread to exit\n");
    myprm.exit_request = OS_TRUE;
    osal_event_set(myprm.thread_event);
    osal_thread_join(handle);

    return OSAL_SUCCESS;
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
static void my_attached_thread(
    void *prm,
    osalEvent done)
{
    MyThreadParameters *myprm;

    /* Save parameter pointer (here we expect parameter structure to be valid while thread runs).
     */
    myprm = (MyThreadParameters*)prm;

    /* Let thread which created this one proceed.
     */
    osal_event_set(done);

    while (OS_TRUE)
    {
        osal_event_wait(myprm->thread_event, OSAL_EVENT_INFINITE);
        if (osal_stop() || myprm->exit_request) {
            break;
        }
        osal_console_write("child thread runs\n");
    }

    osal_console_write("child thread terminated\n");
}

