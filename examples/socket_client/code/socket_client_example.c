/**

  @file    eosal/examples/socket_client/code/socket_client_example.c
  @brief   Socket client example.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Connects a socket to server
  - Create worker thread to trigger custom event once per two seconds.
  - Block in select, print asterix '*' when select is unblocked.
  - Print characters received from the socket to console.
  - Write to socket at key press (trough custom event).

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

/* Select how to connect: TCP socket, TLS socket (OpenSSL, etc) or serial port.
   One of EXAMPLE_USE_TCP_SOCKET, EXAMPLE_USE_TLS_SOCKET or EXAMPLE_USE_SERIAL_PORT.
 */
#define EXAMPLE_USE EXAMPLE_USE_TLS_SOCKET

/* Modify connection parameters here: These apply to different communication types
   EXAMPLE_USE_TCP_SOCKET: Define EXAMPLE_TCP_SOCKET sets TCP/IP address to connect to.
   EXAMPLE_USE_TLS_SOCKET: Define EXAMPLE_TLS_SOCKET sets TCP/IP address to connect to for
   secure sockets.
   EXAMPLE_USE_SERIAL_PORT, define EXAMPLE_SERIAL_PORT: Serial port can be selected using Windows
   style using "COM1", "COM2"... These are mapped to hardware/operating system in device specific
   manner. On Linux port names like "ttyS30,baud=115200" or "ttyUSB0" can be also used.
 */
#define EXAMPLE_TCP_SOCKET "127.0.0.1"
#define EXAMPLE_TLS_SOCKET "192.168.1.220"
#define EXAMPLE_SERIAL_PORT "COM4:,baud=115200"

/* Parameters to start a worker thread.
 */
typedef struct MyThreadParams
{
    osalEvent  myevent;
    volatile os_boolean stopthread;
}
MyThreadParams;

/* If needed for the operating system, EOSAL_C_MAIN macro generates the actual C main() function.
 */
EOSAL_C_MAIN


/**
****************************************************************************************************

  @brief Worker thread.

  The worker thread function sents custom event to allow blocked osal_stream_select() to continue.

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
    MyThreadParams *mythreadprm;
    osalStream handle = OS_NULL;
    osalStatus status;
    os_char buf[64];
    os_memsz n_read;

    mythreadprm = (MyThreadParams*)prm;
    osal_event_set(done);

    while (!mythreadprm->stopthread)
    {
        /* If we habe no connection, try to connect. Give five seconds for network (wifi, etc)
           to start up after boot and at least two tries.
         */
        if (handle == OS_NULL)
        {
            #if EXAMPLE_USE==EXAMPLE_USE_TCP_SOCKET
                handle = osal_stream_open(OSAL_SOCKET_IFACE, EXAMPLE_TCP_SOCKET,
                    OS_NULL, &status, OSAL_STREAM_CONNECT|OSAL_STREAM_SELECT|OSAL_STREAM_TCP_NODELAY);
            #endif
            #if EXAMPLE_USE==EXAMPLE_USE_TLS_SOCKET
                handle = osal_stream_open(OSAL_TLS_IFACE, EXAMPLE_TLS_SOCKET,
                    OS_NULL, &status, OSAL_STREAM_CONNECT|OSAL_STREAM_SELECT|OSAL_STREAM_TCP_NODELAY);
            #endif
            #if EXAMPLE_USE==EXAMPLE_USE_SERIAL_PORT
                handle = osal_stream_open(OSAL_SERIAL_IFACE, EXAMPLE_SERIAL_PORT,
                    OS_NULL, &status, OSAL_STREAM_CONNECT|OSAL_STREAM_SELECT);
            #endif

            if (handle)
            {
                osal_trace("connected");
            }

            else
            {
                osal_debug_error("connect failed");
                os_sleep(100);
                continue;
            }
        }

        /* Block here until something needs attention.
         */
        status = osal_stream_select(&handle, 1, mythreadprm->myevent, OSAL_INFINITE, OSAL_STREAM_DEFAULT);
        if (status)
        {
            osal_debug_error("osal_stream_select failed\n");
            break;
        }

        /* So asterix to indicate that the thread was unblocked.
         */
        osal_console_write("*");

        /* Print data received from the stream to console.
         */
        if (osal_stream_read(handle, buf, sizeof(buf) - 1, &n_read, OSAL_STREAM_DEFAULT))
        {
            osal_debug_error("read: connection broken");
            osal_stream_close(handle, OSAL_STREAM_DEFAULT);
            handle = OS_NULL;
        }
        else if (n_read)
        {
            buf[n_read] ='\0';
            osal_console_write(buf);
        }

        /* Call flush to move data. This is necessary even nothing was written just now. Some stream
           implementetios buffers data internally and this moves buffered data.
         */
        if (osal_stream_flush(handle, OSAL_STREAM_DEFAULT))
        {
            osal_debug_error("flush: connection broken");
            osal_stream_close(handle, OSAL_STREAM_DEFAULT);
            handle = OS_NULL;
        }
    }

    /* Close the socket handle
     */
    osal_stream_close(handle, OSAL_STREAM_DEFAULT);
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
        /* Never call boath osal_socket_initialize() and osal_tls_initialize().
           These use the same underlying library
         */
        osalSecurityConfig security_prm;

        os_memclear(&security_prm, sizeof(security_prm));
        security_prm.trusted_cert_file = "myhome-bundle.crt";

        /* Initialize the transport, socket, TLS, serial, etc..
         */
        osal_tls_initialize(OS_NULL, 0, OS_NULL, 0, &security_prm);
    #endif
    #if EXAMPLE_USE==EXAMPLE_USE_SERIAL_PORT
        osal_serial_initialize();
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
