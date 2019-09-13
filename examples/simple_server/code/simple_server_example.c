/**

  @file    eosal/examples/simple_server/code/simple_server_example.c
  @brief   Socket server example.
  @author  Pekka Lehtikoski
  @version 1.2
  @date    9.9.2019

  Extermely simple communication server: No dynamic memory alloccation, multithreading, socket
  select, etc. Uses single thread loop. Just bare bones. 

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
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
 */
#define EXAMPLE_USE EXAMPLE_USE_TLS_SOCKET

/* Modify connection parameters here: These apply to different communication types
   EXAMPLE_USE_TCP_SOCKET: Define EXAMPLE_TCP_SOCKET_PORT sets unsecured TCP socket port number 
   to listen.
   EXAMPLE_USE_TLS_SOCKET: Define EXAMPLE_TLS_SOCKET_PORT sets secured TCP socket port number 
   to listen.
   EXAMPLE_USE_TLS_SOCKET: Defines EXAMPLE_TLS_SERVER_CERT and EXAMPLE_TLS_SERVER_KEY set path
   to cerver certificate and key files.
   EXAMPLE_USE_SERIAL_PORT, define EXAMPLE_SERIAL_PORT: Serial port can be selected using Windows
   style using "COM1", "COM2"... These are mapped to hardware/operating system in device specific 
   manner. On Linux port names like "ttyS30,baud=115200" or "ttyUSB0" can be also used.
 */
#define EXAMPLE_TCP_SOCKET_PORT "6368"
#define EXAMPLE_TLS_SOCKET_PORT "55555"
#define EXAMPLE_TLS_SERVER_CERT "/coderoot/eosal/examples/simple_server/sllfiles/server.crt"
#define EXAMPLE_TLS_SERVER_KEY "/coderoot/eosal/examples/simple_server/sllfiles/server.key"
#define EXAMPLE_SERIAL_PORT "COM3,baud=115200"

/* Handle for connected stream, and for the stream listening for connections.
 */
static osalStream stream, open_socket;



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
os_int osal_main(
    os_int argc,
    os_char *argv[])
{
    osal_debug_error("here");
return 0;
#if EXAMPLE_USE==EXAMPLE_USE_TCP_SOCKET
    osal_socket_initialize();
    stream = osal_stream_open(OSAL_SOCKET_IFACE, ":" EXAMPLE_TCP_SOCKET_PORT, OS_NULL,
        OS_NULL, OSAL_STREAM_LISTEN|OSAL_STREAM_NO_SELECT);
#endif

#if EXAMPLE_USE==EXAMPLE_USE_TLS_SOCKET
    static osalTLSParam prm = {EXAMPLE_TLS_SERVER_CERT, EXAMPLE_TLS_SERVER_KEY};
    osal_socket_initialize();
    osal_tls_initialize(&prm);
    stream = osal_stream_open(OSAL_TLS_IFACE, ":" EXAMPLE_TLS_SOCKET_PORT, OS_NULL,
        OS_NULL, OSAL_STREAM_LISTEN|OSAL_STREAM_NO_SELECT);
#endif

#if EXAMPLE_USE==EXAMPLE_USE_SERIAL_PORT
    osal_serial_initialize();
    stream = osal_stream_open(OSAL_SERIAL_IFACE, EXAMPLE_SERIAL_PORT, OS_NULL,
        OS_NULL, OSAL_STREAM_LISTEN|OSAL_STREAM_NO_SELECT);
#endif

    if (stream == OS_NULL)
    {
        osal_debug_error("osal_stream_open failed");
    }
    osal_trace("listening for connections");
    open_socket = OS_NULL;

    /* When emulating micro-controller on PC, run loop. Does nothing on real micro-controller.
     */
    osal_simulated_loop(OS_NULL);

    return 0;
}


#if EXAMPLE_USE==EXAMPLE_USE_SERIAL_PORT
/**
****************************************************************************************************

  @brief Loop function to be called repeatedly.

  The osal_loop() function:
  - Reads data received from a serial port prints it to console. 
  - Check for user key pressess and writes those to serial port.

  Note: If user pressess keys faster than these can be written so so fast that they cannot be
        written to serial port, some keys presses are lost. To handle this situation, one should
        check value of nwritten after osal_stream_write() function call. If this is less than
        number of bytes to write, some data is still not written. This could be handled bysetting 
        a write time out by osal_stream_set_parameter(stream, OSAL_STREAM_WRITE_TIMEOUT_MS, 
        <timeout_ms>) function, tough this would block to wait until there is space in outgoing
        buffer.

  @param   prm Void pointer, reserved to pass context structure, etc. 
  @return  None.

****************************************************************************************************
*/
osalStatus osal_loop(
    void *prm)
{
    os_uchar buf[64];
    os_memsz n_read, n_written;
    os_uint c;
    os_int bytes;

    /* Print data from serial port to console.
     */
    if (osal_stream_read(stream, buf, sizeof(buf) - 1, &n_read, OSAL_STREAM_DEFAULT))
    {
        osal_debug_error("read: serial port failed");
        osal_stream_close(stream);
        stream = OS_NULL;
        return;
    }
    else if (n_read)
    {
        buf[n_read] ='\0';
        osal_console_write((os_char*)buf);
    }

    /* And write user key presses to the serial port.
     */
    c = osal_console_read();
    if (c && stream)
    {
        bytes = osal_char_utf32_to_utf8((os_char*)buf, sizeof(buf), c);
        if (osal_stream_write(stream, buf, bytes, &n_written, OSAL_STREAM_DEFAULT))
        {
            osal_debug_error("write: serial port failed");
            osal_stream_close(stream);
            stream = OS_NULL;
            return;
        }
    }

    /* Call flush to move data. This is necessary even nothing was written just now. Some stream 
       implementetios buffers data internally and this moves buffered data.
     */
    osal_stream_flush(stream, OSAL_STREAM_DEFAULT);

    return OSAL_SUCCESS;
}

#else
/**
****************************************************************************************************

  @brief Loop function to be called repeatedly.

  The osal_loop() function:
  - Accepts incoming TCP/TLS socket connection. 
  - If we have a connection:
  -- Reads data received from socket and prints it to console. 
  -- Check for user key pressess and writes those to socket.

  Note: See note for serial communication, same applies here.

  @param   prm Void pointer, reserved to pass context structure, etc. 
  @return  None.

****************************************************************************************************
*/
osalStatus osal_loop(
    void *prm)
{
    os_uchar buf[64];
    os_memsz n_read, n_written;
    os_uint c;
    os_int bytes;
    osalStream accepted_socket;

static os_timer t;
if (os_elapsed(&t, 1000))
{
osal_debug_error("Ukke");
os_get_timer(&t);
}
return OSAL_SUCCESS;    

    osal_socket_maintain();

    /* Try to accept incoming socket connection.
     */
    accepted_socket = osal_stream_accept(stream, OS_NULL, OSAL_STREAM_DEFAULT);
    if (accepted_socket)
    {
        if (open_socket == OS_NULL)
        {
            osal_trace("socket connection accepted");
            open_socket = accepted_socket;
        }
        else
        {
            osal_debug_error("socket already open. This example allows only one connected socket");
            osal_stream_close(accepted_socket);
        }
    }

    /* If we have an open socket, print data from it to console and write user key presses
       to the socket
     */
    if (open_socket)
    {
        if (osal_stream_read(open_socket, buf, sizeof(buf) - 1, &n_read, OSAL_STREAM_DEFAULT))
        {
            osal_debug_error("read: connection broken");
            osal_stream_close(open_socket);
            open_socket = OS_NULL;
        }
        else if (n_read)
        {
            buf[n_read] ='\0';
            osal_console_write((os_char*)buf);
        }

        c = osal_console_read();
        if (c)
        {
            bytes = osal_char_utf32_to_utf8((os_char*)buf, sizeof(buf), c);
            if (osal_stream_write(open_socket, buf, bytes, &n_written, OSAL_STREAM_DEFAULT))
            {
                osal_debug_error("write: connection broken");
                osal_stream_close(open_socket);
                open_socket = OS_NULL;
            }
        }
    }

    /* Call flush to move data. This is necessary even nothing was written just now. Some stream 
       implementetios buffers data internally and this moves buffered data. 
     */
    if (open_socket)
    {
        osal_stream_flush(open_socket, OSAL_STREAM_DEFAULT);
    }

    return OSAL_SUCCESS;
}
#endif


/**
****************************************************************************************************

  @brief Finish with communication.

  The osal_main_cleanup() function closes listening socket port and connected socket port, then
  closes underlying stream library. Notice that the osal_stream_close() function does close does 
  nothing if it is called with NULL argument.

  @param   prm Void pointer, reserved to pass context structure, etc. 
  @return  None.

****************************************************************************************************
*/
void osal_main_cleanup(
    void *prm)
{
    osal_stream_close(open_socket);
    osal_stream_close(stream);

#if EXAMPLE_USE==EXAMPLE_USE_TCP_SOCKET
    osal_socket_shutdown();
#endif

#if EXAMPLE_USE==EXAMPLE_USE_TLS_SOCKET
    osal_tls_shutdown();
    osal_socket_shutdown();
#endif

#if EXAMPLE_USE==EXAMPLE_USE_SERIAL_PORT
    osal_serial_shutdown();
#endif
}
