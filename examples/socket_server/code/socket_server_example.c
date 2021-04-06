/**

  @file    eosal/examples/socket_server/code/socket_server_example.c
  @brief   Socket server example.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Socket server using select.
  This is written for system with multithreading support. Cannot be used with single thread model,
  and not efficient without implemented select. So there is no loop function, etc.

  Multiple sockets are supported. The worker thread accepts incoming sockets and handles
  data transfer to/from the sockets.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"

/* Connection types.
 */
#define EXAMPLE_USE_TCP_SOCKET 0
#define EXAMPLE_USE_TLS_SOCKET 1
#define EXAMPLE_USE_SERIAL_PORT 2

/* What kind of transport: TCP socket or TLS socket (OpenSSL, etc).
   One of EXAMPLE_USE_TCP_SOCKET or EXAMPLE_USE_TLS_SOCKET.
 */
#define EXAMPLE_USE EXAMPLE_USE_TLS_SOCKET

/* Parameters to start a worker thread.
 */
typedef struct MyThreadParams
{
    osalEvent myevent;
    volatile os_boolean stopthread;
}
MyThreadParams;

/* If needed for the operating system, EOSAL_C_MAIN macro generates the actual C main() function.
 */
EOSAL_C_MAIN


/**
****************************************************************************************************

  @brief Worker thread.

  The worker thread function listens for socket connections osal_stream_select() to continue.

  @param   prm Pointer to worker thread parameter structure.
  @param   done Event to set when worker thread has started. Often used to indicate that
           worker thread has made a local copy of parameters.

  @return  None.

****************************************************************************************************
*/
static void mythread_func(
    void *prm,
    osalEvent done)
{
    MyThreadParams *p;
    osalStream handle[OSAL_SOCKET_SELECT_MAX], st;
    osalStatus status;
    osalSelectData selectdata;
    os_int i;
    os_char buf[64];
    os_memsz n_read, n_written;
    const char keypressedtext[] = "<server-key>";

    /* Let thread which created this one proceed.
     */
    p = (MyThreadParams*)prm;
    osal_event_set(done);

    os_memclear(handle, sizeof(handle));

    while (!p->stopthread)
    {
        /* If listening socket is not open, open it now.
         */
        if (handle[0] == OS_NULL)
        {
            #if EXAMPLE_USE==EXAMPLE_USE_TCP_SOCKET
                handle[0] = osal_stream_open(OSAL_SOCKET_IFACE, OS_NULL,
                    OS_NULL, &status, OSAL_STREAM_LISTEN|OSAL_STREAM_SELECT);
            #endif
            #if EXAMPLE_USE==EXAMPLE_USE_TLS_SOCKET
                handle[0] = osal_stream_open(OSAL_TLS_IFACE, OS_NULL,
                    OS_NULL, &status, OSAL_STREAM_LISTEN|OSAL_STREAM_SELECT);
            #endif

            if (handle[0])
            {
                osal_trace("listening socket");
            }

            else
            {
                osal_debug_error("unable to open listening socket");
                os_sleep(100);
                continue;
            }
        }

        /* Select will block here until something worth while happens.
           Like custom event by user key press or someting received from socket, etc.
         */
        status = osal_stream_select(handle, OSAL_SOCKET_SELECT_MAX, p->myevent,
            &selectdata, 0, OSAL_STREAM_DEFAULT);
        if (status)
        {
            osal_debug_error("osal_stream_select failed");
        }

        /* If accepting an incoming socket connection.
         */
        if (selectdata.eventflags & OSAL_STREAM_ACCEPT_EVENT)
        {
            osal_console_write("accept event\n");

            /* Find free handle
             */
            for (i = 1; i < OSAL_SOCKET_SELECT_MAX; i++)
            {
                if (handle[i] == OS_NULL)
                {
                    handle[i] = osal_stream_accept(handle[0], OS_NULL, 0, &status, OSAL_STREAM_DEFAULT);
                    break;
                }
            }
            if (i == OSAL_SOCKET_SELECT_MAX)
            {
                /* Handle table full. Accept and close the socket.
                 */
                osal_debug_error("handle table full");
                st = osal_stream_accept(handle[0], OS_NULL, 0, &status, OSAL_STREAM_DEFAULT);
                osal_stream_close(st, OSAL_STREAM_DEFAULT);
            }
        }

        if (selectdata.eventflags & OSAL_STREAM_CUSTOM_EVENT)
        {
            osal_trace("custom event");

            for (i = 1; i < OSAL_SOCKET_SELECT_MAX; i++)
            {
                st = handle[i];
                if (st == OS_NULL) continue;
                if (osal_stream_write(st, keypressedtext,
                    os_strlen(keypressedtext)-1, &n_written, OSAL_STREAM_DEFAULT))
                {
                    osal_debug_error("write: connection broken");
                    osal_stream_close(st, OSAL_STREAM_DEFAULT);
                    handle[i] = OS_NULL;
                }
            }
        }

        /* Show the event flags. For now, I do not recommend relying on these.
           All socket wrapper implementations may not be complete.
         */
        if (selectdata.eventflags & OSAL_STREAM_CLOSE_EVENT)
        {
            osal_trace("close event");
        }
        if (selectdata.eventflags & OSAL_STREAM_CONNECT_EVENT)
        {
            osal_trace("connect event");
        }
        if (selectdata.eventflags & OSAL_STREAM_READ_EVENT)
        {
            osal_trace("read event");
        }
        if (selectdata.eventflags & OSAL_STREAM_WRITE_EVENT)
        {
            osal_console_write("write event\n");
        }

        /* Check for received data.
         */
        for (i = 1; i < OSAL_SOCKET_SELECT_MAX; i++)
        {
            st = handle[i];
            if (st == OS_NULL) continue;

            if (osal_stream_read(st,
                buf, sizeof(buf) - 1, &n_read, OSAL_STREAM_DEFAULT))
            {
                osal_debug_error("read: connection broken");
                osal_stream_close(st, OSAL_STREAM_DEFAULT);
                handle[selectdata.stream_nr] = OS_NULL;
            }
            else if (n_read)
            {
                buf[n_read] ='\0';
                osal_console_write((os_char*)buf);
            }
        }

        /* Flush all sockets. This is "a must" even no data was written
           just now. Internal stream buffers may hold data to be flushed.
         */
        for (i = 1; i < OSAL_SOCKET_SELECT_MAX; i++)
        {
            st = handle[i];
            if (st == OS_NULL) continue;

            if (osal_stream_flush(st, OSAL_STREAM_DEFAULT))
            {
                osal_stream_close(st, OSAL_STREAM_DEFAULT);
                handle[selectdata.stream_nr] = OS_NULL;
            }
        }
    }

    /* Close all sockets.
     */
    for (i = 0; i < OSAL_SOCKET_SELECT_MAX; i++)
    {
        if (handle[i]) osal_stream_close(handle[i], OSAL_STREAM_DEFAULT);
    }
}


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
    MyThreadParams mythreadprm;
    osalThread *mythread;
    os_uint c;

    /* Initialize the underlying transport library.
     */
#if EXAMPLE_USE==EXAMPLE_USE_TCP_SOCKET
    osal_socket_initialize(OS_NULL, 0, OS_NULL, 0);
#endif
#if EXAMPLE_USE==EXAMPLE_USE_TLS_SOCKET
    osalSecurityConfig security_prm;

    os_memclear(&security_prm, sizeof(security_prm));
    security_prm.server_cert_file = "myhome.crt";
    security_prm.server_key_file = "secret/myhome.key";
    security_prm.share_cert_file = "rootca.crt";
    security_prm.trusted_cert_file = "rootca.crt";

    /* Initialize the transport, socket, TLS, serial, etc..
     */
    osal_tls_initialize(OS_NULL, 0, OS_NULL, 0, &security_prm);
#endif

    /* Create worker thread to do actual coommunication.
     */
    os_memclear(&mythreadprm, sizeof(mythreadprm));
    mythreadprm.myevent = osal_event_create();
    mythread = osal_thread_create(mythread_func, &mythreadprm, OS_NULL, OSAL_THREAD_ATTACHED);

    /* Read keyboard and set event if key pressed.
     */
    do
    {
        c = osal_console_read();
        if (c)
        {
            osal_event_set(mythreadprm.myevent);
        }
        os_timeslice();
    }
    while (c != 27); /* ESC to quit */

    /* Join worker thread to this thread.
     */
    mythreadprm.stopthread = OS_TRUE;
    osal_event_set(mythreadprm.myevent);
    osal_thread_join(mythread);

    /* Cleanup.
     */
    osal_event_delete(mythreadprm.myevent);
    return 0;
}
