/**

  @file    eosal/examples/simple_client/code/simple_client_example.c
  @brief   Simple communication client example.
  @author  Pekka Lehtikoski
  @version 1.2
  @date    9.9.2019

  Client to write something to socket or serial port and read from it. Extermely simple: No
  dynamic memory allocation, multithreading, socket select, etc. Just bare bones.

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
#define EXAMPLE_USE EXAMPLE_USE_TCP_SOCKET

/* Modify connection parameters here: These apply to different communication types
   EXAMPLE_USE_TCP_SOCKET: Define EXAMPLE_TCP_SOCKET sets TCP/IP address to connect to.
   EXAMPLE_USE_TLS_SOCKET: Define EXAMPLE_TLS_SOCKET sets TCP/IP address to connect to for 
   secure sockets.
   EXAMPLE_USE_SERIAL_PORT, define EXAMPLE_SERIAL_PORT: Serial port can be selected using Windows
   style using "COM1", "COM2"... These are mapped to hardware/operating system in device specific 
   manner. On Linux port names like "ttyS30,baud=115200" or "ttyUSB0" can be also used.
 */
#define EXAMPLE_TCP_SOCKET "127.0.0.1:6368"
#define EXAMPLE_TLS_SOCKET "127.0.0.1:55555"
#define EXAMPLE_SERIAL_PORT "COM4:,baud=115200"


/**
****************************************************************************************************

  @brief Process entry point.

  The osal_main() function is OS independent entry point.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
os_int osal_main(
    os_int argc,
    os_char *argv[])
{
    osalStream stream;
    os_uchar buf[64];
    os_memsz n_read, n_written;
    os_int bytes;
    os_uint c;

#if EXAMPLE_USE==EXAMPLE_USE_TCP_SOCKET
    osal_socket_initialize();
    stream = osal_stream_open(OSAL_SOCKET_IFACE, EXAMPLE_TCP_SOCKET, OS_NULL,
        OS_NULL, OSAL_STREAM_CONNECT|OSAL_STREAM_NO_SELECT);
#endif

#if EXAMPLE_USE==EXAMPLE_USE_TLS_SOCKET
    osal_socket_initialize();
    osal_tls_initialize(OS_NULL);
    stream = osal_stream_open(OSAL_TLS_IFACE, EXAMPLE_TLS_SOCKET, OS_NULL,
        OS_NULL, OSAL_STREAM_CONNECT|OSAL_STREAM_NO_SELECT);
#endif

#if EXAMPLE_USE==EXAMPLE_USE_SERIAL_PORT
    osal_serial_initialize();
    stream = osal_stream_open(OSAL_SERIAL_IFACE, EXAMPLE_SERIAL_PORT, OS_NULL,
        OS_NULL, OSAL_STREAM_CONNECT|OSAL_STREAM_NO_SELECT);
#endif

    if (stream == OS_NULL)
    {
        osal_debug_error("osal_stream_open failed");
        return 0;
    }
    osal_trace("stream connected");

    while (OS_TRUE)
    {
        osal_socket_maintain();

        /* Print data received from the stream to console.
         */
        if (osal_stream_read(stream, buf, sizeof(buf) - 1, &n_read, OSAL_STREAM_DEFAULT))
        {
            osal_debug_error("read: connection broken");
            break;
        }
        else if (n_read)
        {
            buf[n_read] ='\0';
            osal_console_write((os_char*)buf);
        }

        /* And write user key presses to the stream.
         */
        c = osal_console_read();
        if (c)
        {
            bytes = osal_char_utf32_to_utf8((os_char*)buf, sizeof(buf), c);
            if (osal_stream_write(stream, buf, bytes, &n_written, OSAL_STREAM_DEFAULT))
            {
                osal_debug_error("write: connection broken");
                break;
            }
        }

        /* Call flush to move data. This is necessary even nothing was written just now. Some stream 
           implementetios buffers data internally and this moves buffered data.
         */
        if (osal_stream_flush(stream, OSAL_STREAM_DEFAULT))
        {
            osal_debug_error("flush: connection broken");
            break;
        }

        os_timeslice();
    }

    osal_stream_close(stream);
    stream = OS_NULL;

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

    return 0;
}
