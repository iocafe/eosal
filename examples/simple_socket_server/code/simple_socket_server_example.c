/**

  @file    eosal/examples/simple_socket_server/code/simple_socket_server_example.c
  @brief   Socket server example.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    7.12.2017

  Echo back server. Extermely simple: No dynamic memory alloccation, multithreading, socket
  select, etc. Just bare bones.

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

static osalStream stream, accepted_socket, open_socket;

void example_setup(void);
void example_cleanup(void);
void example_loop(void);


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
    example_setup();

    while (OS_TRUE)
    {
        example_loop();
        os_timeslice();
    }

    example_cleanup();
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
void example_setup(void)
{
#if EXAMPLE_USE==EXAMPLE_USE_TCP_SOCKET
    osal_socket_initialize();
    stream = osal_stream_open(OSAL_SOCKET_IFACE, ":6368", OS_NULL,
//    stream = osal_stream_open(OSAL_SOCKET_IFACE, ":6001", OS_NULL,
        OS_NULL, OSAL_STREAM_LISTEN|OSAL_STREAM_NO_SELECT);
#endif

#if EXAMPLE_USE==EXAMPLE_USE_TLS_SOCKET
    static osalTLSParam prm = {
       "/coderoot/eosal/examples/simple_socket_server/sllfiles/server.crt",
       "/coderoot/eosal/examples/simple_socket_server/sllfiles/server.key"};

    osal_socket_initialize();
    osal_tls_initialize(&prm);
    stream = osal_stream_open(OSAL_TLS_IFACE, ":55555", OS_NULL,
        OS_NULL, OSAL_STREAM_LISTEN|OSAL_STREAM_NO_SELECT);
#endif

#if EXAMPLE_USE==EXAMPLE_USE_SERIAL_PORT
    osal_serial_initialize();
    stream = osal_stream_open(OSAL_SERIAL_IFACE, "COM3:", OS_NULL,
        OS_NULL, OSAL_STREAM_LISTEN|OSAL_STREAM_NO_SELECT);
#endif

    if (stream == OS_NULL)
    {
        osal_debug_error("osal_stream_open failed");
    }
    osal_trace("listening for socket connections");
    open_socket = OS_NULL;
}

void example_loop(void)
{
    os_uchar buf[64], *pos;
    os_memsz n_read, n_written;

    osal_socket_maintain();

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

    if (open_socket)
    {
        if (osal_stream_read(open_socket, buf, sizeof(buf), &n_read, OSAL_STREAM_DEFAULT))
        {
            osal_debug_error("socket connection broken");
            osal_stream_close(open_socket);
            open_socket = OS_NULL;
        }
        else
        {
            pos = buf;
            while (n_read > 0)
            {
                if (osal_stream_write(open_socket, pos, n_read, &n_written, OSAL_STREAM_DEFAULT))
                {
                    osal_stream_close(open_socket);
                    open_socket = OS_NULL;
                    break;
                }
                n_read -= n_written;
                pos += n_written;
                if (n_read == 0) break;
                os_timeslice();
            }
        }
    }
}

void example_cleanup(void)
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
