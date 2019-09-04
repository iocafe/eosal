/**

  @file    eosal/examples/simple_socket_client/code/simple_socket_client_example.c
  @brief   Simple communication client example.
  @author  Pekka Lehtikoski
  @version 1.1
  @date    7.8.2019

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

#define EXAMPLE_USE_TCP_SOCKET 0
#define EXAMPLE_USE_TLS_SOCKET 1
#define EXAMPLE_USE_SERIAL_PORT 2

/* Select how to connect: TCP socket, TLS socket (OpenSSL, etc) or serial port.
 */
#define EXAMPLE_USE EXAMPLE_USE_TCP_SOCKET


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
    os_uchar buf[64], *pos, testdata[] = "?-testdata*", c = 'a';
    os_memsz n_read, n_written;
    os_int n;
    os_timer timer;

#if EXAMPLE_USE==EXAMPLE_USE_TCP_SOCKET
    osal_socket_initialize();
    stream = osal_stream_open(OSAL_SOCKET_IFACE, "192.168.1.221:6001", OS_NULL,
        OS_NULL, OSAL_STREAM_CONNECT|OSAL_STREAM_NO_SELECT);
#endif

#if EXAMPLE_USE==EXAMPLE_USE_TLS_SOCKET
    osal_socket_initialize();
    osal_tls_initialize(OS_NULL);
    stream = osal_stream_open(OSAL_TLS_IFACE, "127.0.0.1:55555", OS_NULL,
        OS_NULL, OSAL_STREAM_CONNECT|OSAL_STREAM_NO_SELECT);
#endif

#if EXAMPLE_USE==EXAMPLE_USE_SERIAL_PORT
    osal_serial_initialize();
    stream = osal_stream_open(OSAL_SERIAL_IFACE, "COM3:", OS_NULL,
        OS_NULL, OSAL_STREAM_CONNECT|OSAL_STREAM_NO_SELECT);
#endif

    if (stream == OS_NULL)
    {
        osal_debug_error("osal_stream_open failed");
        return 0;
    }
    osal_trace("stream connected");

    os_get_timer(&timer);
    while (OS_TRUE)
    {
#if EXAMPLE_USE!=EXAMPLE_USE_SERIAL_PORT
        osal_socket_maintain();
#endif

        if (os_elapsed(&timer, 200))
        {
            testdata[0] = c;
            if (c++ == 'z') c = 'a';
            pos = testdata;
            n = os_strlen((os_char*)testdata) - 1;

            while (n > 0)
            {
                if (osal_stream_write(stream, pos, n, &n_written, OSAL_STREAM_DEFAULT))
                {
                    osal_debug_error("connection broken");
                    goto getout;
                }

                n -= n_written;
                pos += n_written;
                if (n == 0) break;
                os_timeslice();
            }
            os_get_timer(&timer);
        }

        os_memclear(buf, sizeof(buf));
        if (osal_stream_read(stream, buf, sizeof(buf)-1, &n_read, OSAL_STREAM_DEFAULT))
        {
            osal_debug_error("connection broken");
            goto getout;
        }

        if (n_read > 0)
        {
            osal_console_write((os_char*)buf);
        }

        os_timeslice();
    }

getout:
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
