/**

  @file    eosal/examples/serial_io/code/serial_io_example.c
  @brief   Serial communication example.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    7.10.2018

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"

typedef struct MyThreadParams
{
    osalEvent  myevent;
    os_boolean stopthread;
}
MyThreadParams;

static void mythread_func(
	void *prm,
    osalEvent done)
{
    MyThreadParams *p;

    p = (MyThreadParams*)prm;
    osal_event_set(done);

    while (!p->stopthread)
    {
        os_sleep(10000);
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
    os_char mystr[] = "0.eppu", buf[64], *mystr2;
    os_memsz n_read, n_written;
    osalEvent  myevent;
    MyThreadParams mythreadprm;
    osalThreadHandle *mythread = OS_NULL;

    /* Initialize the serial communication library (not needed for all platforms) 
     */
    osal_serial_initialize();

    /* Open serial port.
     */
    handle = osal_stream_open(OSAL_SERIAL_IFACE, "COM3:", 
        OS_NULL, &status, OSAL_STREAM_CONNECT|OSAL_STREAM_SELECT);

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

    status = osal_stream_write(handle, mystr,
        os_strlen(mystr)-1, &n_written, OSAL_STREAM_DEFAULT);


    while (OS_TRUE)
    {
        if (mystr[0]++ == '9') mystr[0] = '0';

        status = osal_stream_select(&handle, 1, myevent, &selectdata, 20000, OSAL_STREAM_DEFAULT);
        if (status)
        {
	        osal_console_write("osal_stream_select failed\n");
            break;
        }

        if (selectdata.eventflags & OSAL_STREAM_CUSTOM_EVENT)
        {
            osal_console_write("custom event\n");
            status = osal_stream_write(handle, mystr,
                os_strlen(mystr)-1, &n_written, OSAL_STREAM_DEFAULT);
        }

        if (selectdata.eventflags & OSAL_STREAM_TIMEOUT_EVENT)
        {
            osal_console_write("timeout\n");
        }

        if (selectdata.eventflags & OSAL_STREAM_READ_EVENT)
        {
            osal_console_write("read event: ");
            os_memclear(buf, sizeof(buf));
            status = osal_stream_read(handle, buf, sizeof(buf) - 1, &n_read, OSAL_STREAM_DEFAULT);
            if (status) break;
            osal_console_write(buf);
            osal_console_write("\n");

            if (n_read)
            {
                switch (buf[0])
                {
                    case 'a':
                    case 'A':
                        mystr2 = "Abba";
                        break;

                    case 'b':
                    case 'B':
                        mystr2 = "Bansku";
                        break;

                    default:
                        mystr2 = "Duudeli";
                        break;
                }

                status = osal_stream_write(handle, mystr2,
                    os_strlen(mystr2)-1, &n_written, OSAL_STREAM_DEFAULT);
            }
        }

        if (selectdata.eventflags & OSAL_STREAM_WRITE_EVENT)
        {
            osal_console_write("write event\n");
            /* status = osal_stream_write(handle, mystr, os_strlen(mystr)-1, &n_written, OSAL_STREAM_DEFAULT); */
        }
    }

    osal_stream_close(handle);

    /* Join worker thread to this thread.
     */
    mythreadprm.stopthread = OS_TRUE;
    if (mythread) osal_thread_join(mythread);

    /* Delete an event.
     */
    osal_event_delete(myevent);

    /* Shut down serial com library and return.
     */
    osal_serial_shutdown();
    return 0;
}
