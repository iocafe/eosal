/**

  @file    eosal/examples/simple_server/code/simple_server_example.c
  @brief   Socket server example.
  @author  Pekka Lehtikoski
  @version 1.2
  @date    8.1.2020

  Simple communication server: No dynamic memory alloccation, multithreading, socket
  select, etc. Uses single thread loop. Just bare bones.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/

/* Force tracing on for this source file.
 */
#undef OSAL_TRACE
#define OSAL_TRACE 3

#include "eosalx.h"

/* Connection types.
 */
#define EXAMPLE_USE_TCP_SOCKET 0
#define EXAMPLE_USE_TLS_SOCKET 1
#define EXAMPLE_USE_SERIAL_PORT 2

/* Select how to connect: TCP socket, TLS socket (OpenSSL, etc) or serial port.
 * EXAMPLE_USE_TCP_SOCKET, EXAMPLE_USE_TLS_SOCKET or EXAMPLE_USE_SERIAL_PORT
 */
#define EXAMPLE_USE EXAMPLE_USE_TLS_SOCKET

/* EXAMPLE_USE_SERIAL_PORT, define EXAMPLE_SERIAL_PORT: Serial port can be selected using Windows
   style using "COM1", "COM2"... These are mapped to hardware/operating system in device specific
   manner. On Linux port names like "ttyS30,baud=115200" or "ttyUSB0" can be also used.
 */
#define EXAMPLE_SERIAL_PORT "COM3,baud=115200"


/* Handle for connected stream, and for the stream listening for connections.
 */
static osalStream stream, mystream;

/* If needed for the operating system, EOSAL_C_MAIN macro generates the actual C main() function.
 */
EOSAL_C_MAIN


/**
****************************************************************************************************

  @brief Process entry point.

  The osal_main() function is OS independent entry point.

  The function initializes used stream library and either opens a serial port or creates
  listening TCP/TLS socket.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
osalStatus osal_main(
    os_int argc,
    os_char *argv[])
{
    /* Initialize the underlying transport library.
     */
#if EXAMPLE_USE==EXAMPLE_USE_TCP_SOCKET
    osal_socket_initialize(OS_NULL, 0, OS_NULL, 0);
#endif
#if EXAMPLE_USE==EXAMPLE_USE_TLS_SOCKET
    osalSecurityConfig security_prm;

    os_memclear(&security_prm, sizeof(security_prm));
    security_prm.server_cert_file = "rootca.crt";
    security_prm.server_key_file = "secret/rootca.key";
    security_prm.root_cert_file = security_prm.server_cert_file;

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

    /* When emulating micro-controller on PC, run loop. Does nothing on real micro-controller.
     */
    osal_simulated_loop(OS_NULL);

    return 0;
}


/**
****************************************************************************************************

  @brief Loop function to be called repeatedly.

  The osal_loop() function:
  - Accepts incoming TCP/TLS socket connection.
  - If we have a connection:
  -- Reads data received from socket and prints it to console.
  -- Check for user key pressess and writes those to socket.

  Note: See note for serial communication, same applies here.

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
    os_uint c;
    os_int bytes;
    osalStream accepted_socket;

    /* Some socket library implementations need this, for DHCP, etc.
     */
    osal_socket_maintain();

    /* If we are not listening for socket connection or have serial port open, try to open now.
     */
    if (stream == OS_NULL)
    {
        #if EXAMPLE_USE==EXAMPLE_USE_TCP_SOCKET
            stream = osal_stream_open(OSAL_SOCKET_IFACE, OS_NULL, OS_NULL,
                OS_NULL, OSAL_STREAM_LISTEN|OSAL_STREAM_NO_SELECT);
        #endif
        #if EXAMPLE_USE==EXAMPLE_USE_TLS_SOCKET
            stream = osal_stream_open(OSAL_TLS_IFACE, OS_NULL, OS_NULL,
                OS_NULL, OSAL_STREAM_LISTEN|OSAL_STREAM_NO_SELECT);
        #endif
        #if EXAMPLE_USE==EXAMPLE_USE_SERIAL_PORT
            stream = osal_stream_open(OSAL_SERIAL_IFACE, EXAMPLE_SERIAL_PORT, OS_NULL,
                OS_NULL, OSAL_STREAM_LISTEN|OSAL_STREAM_NO_SELECT);
        #endif

        if (stream)
        {
            osal_trace("serial or listening socket port opened");
        }
    }

#if EXAMPLE_USE==EXAMPLE_USE_SERIAL_PORT
    mystream = stream;
#else
    /* Try to accept incoming socket connection.
     */
    if (stream)
    {
        accepted_socket = osal_stream_accept(stream, OS_NULL, 0, OS_NULL, OSAL_STREAM_DEFAULT);
        if (accepted_socket)
        {
            if (mystream)
            {
                osal_stream_close(mystream, OSAL_STREAM_DEFAULT);

                osal_debug_error("socket already open. this example allows only one socket. "
                    "old connection closed");
            }
            mystream = accepted_socket;
            osal_trace("socket accepted");
        }
    }
#endif

    /* If we have an open stream, print data from it to console.
     */
    if (mystream)
    {
        if (osal_stream_read(mystream, buf, sizeof(buf) - 1, &n_read, OSAL_STREAM_DEFAULT))
        {
            osal_debug_error("read: connection broken");
            osal_stream_close(mystream, OSAL_STREAM_DEFAULT);
            mystream = OS_NULL;
        }
        else if (n_read)
        {
            buf[n_read] ='\0';
            osal_console_write((os_char*)buf);
        }
    }

    /* And write user key presses to the stream.
     */
    if (mystream)
    {
        c = osal_console_read();
        if (c)
        {
            bytes = osal_char_utf32_to_utf8(buf, sizeof(buf), c);
            if (osal_stream_write(mystream, buf, bytes, &n_written, OSAL_STREAM_DEFAULT))
            {
                osal_debug_error("write: connection broken");
                osal_stream_close(mystream, OSAL_STREAM_DEFAULT);
                mystream = OS_NULL;
            }
        }
    }

    /* Call flush to move data. This is necessary even nothing was written just now. Some stream
       implementetios buffers data internally and this moves buffered data.
     */
    if (mystream)
    {
        if (osal_stream_flush(mystream, OSAL_STREAM_DEFAULT))
        {
            osal_stream_close(mystream, OSAL_STREAM_DEFAULT);
            mystream = OS_NULL;
        }
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Finish with communication.

  The osal_main_cleanup() function closes listening socket port and connected socket port, then
  closes underlying stream library. Notice that the osal_stream_close() function does close does
  nothing if it is called with NULL argument.

  @param   app_context Void pointer, reserved to pass context structure, etc.
  @return  None.

****************************************************************************************************
*/
void osal_main_cleanup(
    void *app_context)
{
    osal_stream_close(mystream, OSAL_STREAM_DEFAULT);
    osal_stream_close(stream, OSAL_STREAM_DEFAULT);

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
