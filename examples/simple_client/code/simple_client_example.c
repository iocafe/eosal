/**

  @file    eosal/examples/simple_client/code/simple_client_example.c
  @brief   Simple communication client example.
  @author  Pekka Lehtikoski
  @version 1.2
  @date    8.1.2020

  Client to write something to socket or serial port and read from it. Extermely simple: No
  dynamic memory allocation, multithreading, socket select, etc. Just bare bones.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

/* Force tracing on for this source file.
 */
#include "/coderoot/mypasswords.h"
#undef OSAL_TRACE
#define OSAL_TRACE 3

#include "eosalx.h"

/* Connection types.
 */
#define EXAMPLE_USE_TCP_SOCKET 0
#define EXAMPLE_USE_TLS_SOCKET 1
#define EXAMPLE_USE_SERIAL_PORT 2

/* Select how to connect: TCP socket, TLS socket (OpenSSL, etc) or serial port.
   One of EXAMPLE_USE_TCP_SOCKET, EXAMPLE_USE_TLS_SOCKET or EXAMPLE_USE_SERIAL_PORT.
 */
#define EXAMPLE_USE EXAMPLE_USE_TCP_SOCKET

/* Modify connection parameters here: These apply to different communication types
   EXAMPLE_USE_TCP_SOCKET: Define EXAMPLE_TCP_SOCKET sets TCP/IP address to connect to.
   EXAMPLE_USE_TLS_SOCKET: Define EXAMPLE_TLS_SOCKET sets TCP/IP address to connect to for 
   secure sockets.
   EXAMPLE_USE_SERIAL_PORT, define EXAMPLE_SERIAL_PORT: Serial port can be selected using Windows
   style using "COM1", "COM2"... These are mapped to hardware/operating system in device specific 
   manner. On Linux port names like "ttyS30,baud=115200" or "ttyUSB0" can be also used.
 */
/*#define EXAMPLE_TCP_SOCKET "127.0.0.1:6368"*/
#define EXAMPLE_TCP_SOCKET "192.168.1.101:6368"

#define EXAMPLE_TLS_SOCKET "127.0.0.1:6369"
#define EXAMPLE_SERIAL_PORT "COM4:,baud=115200"

static osalStream stream;


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
    osalWifiNetwork wifi;

    os_sleep(7000);

    os_memclear(&wifi, sizeof(wifi));

    wifi.wifi_net_name = MYNETNAME;
    wifi.wifi_net_password = MYPASSWORD;
    osal_initialize_net_state();

    /* Initialize underlying transport library.
     */
    #if EXAMPLE_USE==EXAMPLE_USE_TCP_SOCKET
    osal_trace("HERE 103");
        osal_socket_initialize(OS_NULL, 0, &wifi, 1);
    #endif
    #if EXAMPLE_USE==EXAMPLE_USE_TLS_SOCKET
        osalSecurityConfig security_prm;

        os_memclear(&security_prm, sizeof(security_prm));
        security_prm.client_cert_chain_file = "myhome-bundle.crt";

        /* Initialize the transport, socket, TLS, serial, etc..
         */
        osal_tls_initialize(OS_NULL, 0, OS_NULL, 0, &security_prm);
    #endif
    #if EXAMPLE_USE==EXAMPLE_USE_SERIAL_PORT
        osal_serial_initialize();
    #endif

    /* All microcontroller do not clear memory at soft reboot.
     */
    stream = OS_NULL;

    /* When emulating micro-controller on PC, run loop. Just save context pointer on
       real micro-controller.
     */
    osal_simulated_loop(OS_NULL);
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Loop function to be called repeatedly.

  The osal_loop() function:
  - If we have a connection:
  -- Reads data received from socket and prints it to console.
  -- Check for user key pressess and writes those to socket.

  @param   app_context Void pointer, reserved to pass context structure, etc.
  @return  The function returns OSAL_SUCCESS to continue running. Other return values are
           to be interprened as reboot on micro-controller or quit the program on PC computer.

****************************************************************************************************
*/
osalStatus osal_loop(
    void *app_context)
{
    os_char buf[64];
    os_memsz n_read, n_written;
    os_int bytes;
    os_uint c;

    /* Some socket library implementations need this, for DHCP, etc.
     */
    osal_socket_maintain();

    /* Connect. Give five seconds for network (wifi, etc) to start up
       after boot and at least two tries.
     */
    if (stream == OS_NULL)
    {
        #if EXAMPLE_USE==EXAMPLE_USE_TCP_SOCKET
            stream = osal_stream_open(OSAL_SOCKET_IFACE, EXAMPLE_TCP_SOCKET, OS_NULL,
                OS_NULL, OSAL_STREAM_CONNECT|OSAL_STREAM_NO_SELECT);
        #endif
        #if EXAMPLE_USE==EXAMPLE_USE_TLS_SOCKET
            stream = osal_stream_open(OSAL_TLS_IFACE, EXAMPLE_TLS_SOCKET, OS_NULL,
                OS_NULL, OSAL_STREAM_CONNECT|OSAL_STREAM_NO_SELECT);
        #endif
        #if EXAMPLE_USE==EXAMPLE_USE_SERIAL_PORT
            stream = osal_stream_open(OSAL_SERIAL_IFACE, EXAMPLE_SERIAL_PORT, OS_NULL,
                OS_NULL, OSAL_STREAM_CONNECT|OSAL_STREAM_NO_SELECT);
        #endif

        if (stream)
        {
            osal_trace("stream connected");
        }
    }

    /* Print data received from the stream to console.
     */
    if (stream)
    {
        if (osal_stream_read(stream, buf, sizeof(buf) - 1, &n_read, OSAL_STREAM_DEFAULT))
        {
            osal_debug_error("read: connection broken");
            osal_stream_close(stream, OSAL_STREAM_DEFAULT);
            stream = OS_NULL;
        }
        else if (n_read)
        {
            buf[n_read] ='\0';
            osal_console_write((os_char*)buf);
        }
    }

    /* And write user key presses to the stream.
     */
    if (stream)
    {
        c = osal_console_read();
        if (c)
        {
            bytes = osal_char_utf32_to_utf8(buf, sizeof(buf), c);
            if (osal_stream_write(stream, buf, bytes, &n_written, OSAL_STREAM_DEFAULT))
            {
                osal_debug_error("write: connection broken");
                osal_stream_close(stream, OSAL_STREAM_DEFAULT);
                stream = OS_NULL;
            }
        }
    }

    /* Call flush to move data. This is necessary even nothing was written just now. Some stream
       implementations buffers data internally and this moves buffered data.
     */
    if (stream)
    {
        if (osal_stream_flush(stream, OSAL_STREAM_DEFAULT))
        {
            osal_debug_error("flush: connection broken");
            osal_stream_close(stream, OSAL_STREAM_DEFAULT);
            stream = OS_NULL;
        }
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Finished with the application, clean up.

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
    osal_stream_close(stream, OSAL_STREAM_DEFAULT);
    stream = OS_NULL;

#if EXAMPLE_USE==EXAMPLE_USE_TCP_SOCKET
    osal_socket_shutdown();
#endif

#if EXAMPLE_USE==EXAMPLE_USE_TLS_SOCKET
    osal_tls_shutdown();
#endif

#if EXAMPLE_USE==EXAMPLE_USE_SERIAL_PORT
    osal_serial_shutdown();
#endif
}
