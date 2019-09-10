/**

  @file    eosal/examples/socket_client/code/socket_client_example.c
  @brief   Socket client example.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.9.2019

  Connects a socket to server
  - Create worker thread to trigger custom event once per two seconds.
  - Block in select, print asterix '*' when select is unblocked.
  - Print characters received from the socket to console.
  - Write characters typed by use to socket at custom event.
  
  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"

/* Modify connection parameters here: These apply to different communication types
 */
#define EXAMPLE_TCP_SOCKET "127.0.0.1:6368"

/* Parameters to start a worter thread. 
 */
typedef struct MyThreadParams
{
    osalEvent  myevent;
    os_boolean stopthread;
}
MyThreadParams;


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
    MyThreadParams *p;

    p = (MyThreadParams*)prm;
    osal_event_set(done);

    while (!p->stopthread)
    {
        os_sleep(2000);
        osal_event_set(p->myevent);
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
os_int osal_main(
    os_int argc,
    os_char *argv[])
{
    osalStream handle;
    osalStatus status;
    osalSelectData selectdata;
    os_char buf[64], *p;
    os_memsz n_read, n_written;
    os_int bytes, n;
    os_uint c;
    osalEvent  myevent;
    MyThreadParams mythreadprm;
    osalThreadHandle *mythread;

    /* Initialize socket library and connect a socket. Notice flag OSAL_STREAM_TCP_NODELAY.
       If OSAL_STREAM_TCP_NODELAY is given, the nagle algorithm is disabled and writes
       are buffered until enough data for a packet collected, or osal_stream_flush is
       called. This is very useful for fast IO, etc, but not so for data transfer over
       internet, like user interfaces.
     */
    osal_socket_initialize();
    handle = osal_stream_open(OSAL_SOCKET_IFACE, EXAMPLE_TCP_SOCKET,
        OS_NULL, &status, OSAL_STREAM_CONNECT|OSAL_STREAM_SELECT|OSAL_STREAM_TCP_NODELAY);
    if (status)
    {
	    osal_console_write("osal_stream_open failed\n");
        return 0;
    }

    /* Create an event. Select will react to this event.
     */
    myevent = osal_event_create();

    /* Create worker thread to set the event created above periodically.
     */
    os_memclear(&mythreadprm, sizeof(mythreadprm));
    mythreadprm.myevent = myevent;
    mythread = osal_thread_create(mythread_func, &mythreadprm,
	    OSAL_THREAD_ATTACHED, 0, "mythread");

    while (OS_TRUE)
    {
        /* Block here until something needs attention. 
         */
        status = osal_stream_select(&handle, 1, myevent, &selectdata, 0, OSAL_STREAM_DEFAULT);
        if (status)
        {
	        osal_debug_error("osal_stream_select failed\n");
            break;
        }

        /* So asterix to indicate that the thread was unblocked.
         */
        osal_console_write("*");

        /* Handle custom event set by the worker thread.
         */
        if (selectdata.eventflags & OSAL_STREAM_CUSTOM_EVENT)
        {
            osal_console_write("<custom event>");

            /* At custom event, write user key presses to the stream.
             */
            p = buf;
            n = 0;
            while ((c = osal_console_read()))
            {
                bytes = osal_char_utf32_to_utf8(p, sizeof(buf) - n, c);
                p += bytes;
                n += bytes;
                if (n >= sizeof(buf) - 4) break;
            }

            if (osal_stream_write(handle, buf, n, &n_written, OSAL_STREAM_DEFAULT))
            {
                osal_debug_error("write: connection broken");
                break;
            }
        }

        /* Handle close event. This should not be really necessary. If socket is broken
           the osal_stream_read() will return an error.
         */
        if (selectdata.eventflags & OSAL_STREAM_CLOSE_EVENT)
        {
            osal_console_write("close event\n");
            break;
        }

        /* Handle the connect event. Also strictly not necessary. If the socket is still
           connecting, osal_stream_write() should return with nwritten = 0.
         */
        if (selectdata.eventflags & OSAL_STREAM_CONNECT_EVENT)
        {
            osal_console_write("connect event\n");

            if (selectdata.errorcode)
            {
                osal_debug_error("connect failed\n");
                break;
            }
        }

        /* Print data received from the stream to console.
         */
        if (osal_stream_read(handle, buf, sizeof(buf) - 1, &n_read, OSAL_STREAM_DEFAULT))
        {
            osal_debug_error("read: connection broken");
            break;
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
            break;
        }
    }

    /* Close the socket handle
     */
    osal_stream_close(handle);

    /* Join worker thread to this thread.
     */
    mythreadprm.stopthread = OS_TRUE;
    osal_thread_join(mythread);

    /* Delete an event.
     */
    osal_event_delete(myevent);

    /* Clean up the socket library.
     */
    osal_socket_shutdown();
    return 0;
}
