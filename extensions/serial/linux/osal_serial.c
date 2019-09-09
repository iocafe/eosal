/**

  @file    serial/linux/osal_serial.c
  @brief   OSAL serials API linux implementation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    3.6.2019

  OSAL serial port wrapper implementation for Linux.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#define POSIX_C_SOURCE 200112L

#include "eosalx.h"
#if OSAL_SERIAL_SUPPORT
#include <termios.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/* Use pselect(), POSIX.1-2001 version. According to earlier standards, include <sys/time.h>
   and <sys/types.h>. These would be needed with select instead of sys/select.h if pselect
   is not available.
 */
#include <sys/select.h>


/** Linux specific serial point state data structure. OSAL functions cast their own
    structure pointers to osalStream pointers.
 */
typedef struct osalSerial
{
    /** A stream structure must start with this generic stream header structure, which contains
        parameters common to every stream.
     */
    osalStreamHeader hdr;

    /** Operating system's serial handle.
	 */
    int handle;

    /** Stream open flags. Flags which were given to osal_serial_open()
        function. 
	 */
	os_int open_flags;

    /** OS_TRUE if last write to serial has been blocked.
     */
    os_boolean write_blocked;
}
osalSerial;

/* Pair numeric baud rate (9600, etc) to linux setting define (B9600, etc).
 */
typedef struct
{
    os_int baud;
    tcflag_t flag;
}
osalBaudChoice;

/* Numeric baud rate, linux setting define pairs.
 */
static const osalBaudChoice osal_baud_list[] = {
    {1200, B1200},
    {2400, B2400},
    {4800, B4800},
    {9600, B9600},
    {19200, B19200},
    {38400, B38400},
    {57600, B57600},
    {115200, B115200},
    {230400, B230400},
    {460800, B460800},
    {500000, B500000},
    {576000, B576000},
    {921600, B921600},
    {1000000, B1000000},
    {1152000, B1152000},
    {1500000, B1500000},
    {2000000, B2000000},
    {2500000, B2500000},
    {2500000, B2500000},
    {3000000, B3000000},
    {3500000, B3500000},
    {4000000, B4000000}};

/* Number of elements in osal_baud_list array.
 */
#define OSA_NRO_BAUD_CHOICES (sizeof(osal_baud_list)/sizeof(osalBaudChoice))


/* Prototypes for forward referred static functions.
 */
static void osal_get_linux_serial_port_name(
    const os_char **parameters,
    os_char *portname,
    os_memsz portname_sz);


/**
****************************************************************************************************

  @brief Open a serial port.
  @anchor osal_serial_open

  The osal_serial_open() function opens a serial port.

  Example:
    osalStream handle;
    handle = osal_serial_open("ttyS30,baud=115200", OS_NULL, OS_NULL, OSAL_STREAM_SELECT);
    if (handle == OS_NULL)
    {
        osal_debug_error("Unable to open serial port");
        ...
    }

  @param  parameters Serial port name and parameters, for example "COM5,baud=115200" or
          "ttyUSB0,baud=57600". The parameters string must beging with serial port name.
          It can be either Windows like COMx port name or linux device name without preceeding
          "/dev/". Windows like COM port names COM1 ... COM4 in are interprented as "/dev/tty0",
           "/dev/tty1", "/dev/tty2" and "/dev/tty3" respectively. Similarly names COM5 ...
          COM8 correspond to "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyUSB2" and "/dev/ttyUSB3".
          If port name in parameters string isn't windows like COM port name, it is simply
          prefixed by "/dev/" and to match linux device name.
          The port name can be followed by com port settings, in format "name=value".
          These settings are separated from serial port name and other setting by comma.
          Currectly supported settings are baud=<baudrate> and parity=none/odd/even.

  @param  option Not used for serial ports, set OS_NULL.

  @param  status Pointer to integer into which to store the function status code. Value
		  OSAL_SUCCESS (0) indicates success and all nonzero values indicate an error.
          See @ref osalStatus "OSAL function return codes" for full list.
		  This parameter can be OS_NULL, if no status code is needed. 

  @param  flags Flags for creating the serial. Bit fields, combination of:
          - OSAL_STREAM_NO_SELECT: Open serial without select functionality.
          - OSAL_STREAM_SELECT: Open serial with select functionality (default).

          At the moment, select support flag has no impact on Linux. If define
          OSAL_SERIAL_SELECT_SUPPORT is 1 and select is called, it works. Anyhow flag should
          be set correctly for compatibility with other operating systems. If there are
          flags which are unknown to this function, these are simply ignored.
		  See @ref osalStreamFlags "Flags for Stream Functions" for full list of stream flags.

  @return Stream pointer representing the serial port, or OS_NULL if the function failed.

****************************************************************************************************
*/
osalStream osal_serial_open(
    const os_char *parameters,
	void *option,
	osalStatus *status,
	os_int flags)
{
    osalSerial *myserial = OS_NULL;
    int handle = -1;
    os_char portname[64];
    const os_char *v;
    struct termios serialparams;
    tcflag_t baud, parity;
    os_long baudrate;
    os_int i;
    osalStatus rval = OSAL_STATUS_FAILED;

    /* Open the serial port.
     */
    osal_get_linux_serial_port_name(&parameters, portname, sizeof(portname));
    handle = open(portname, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (handle < 0)
    {
        goto getout;
    }

    /* Baud rate.
     */
    baudrate = osal_string_get_item_int(parameters, "baud", 115200, OSAL_STRING_DEFAULT);
    baud = B115200;
    for (i = 0; i < OSA_NRO_BAUD_CHOICES; i++)
    {
        if (osal_baud_list[i].baud == baudrate)
        {
            baud = osal_baud_list[i].flag;
            break;
        }
    }
    osal_debug_assert(i < OSA_NRO_BAUD_CHOICES);

    /* Parity.
     */
    v = osal_string_get_item_value(parameters, "parity", OS_NULL, OSAL_STRING_DEFAULT);
    parity = 0;
    if (!os_strnicmp(v, "even", 4))
    {
        parity = PARENB;
    }
    else if (!os_strnicmp(v, "odd", 3))
    {
        parity = PARENB|PARODD;
    }

    /* Configue the serial port.
     */
    os_memclear(&serialparams, sizeof(serialparams));
    serialparams.c_cflag = CLOCAL | CREAD | baud |  CS8 | parity;
    serialparams.c_iflag = IGNPAR;
    //if (parity) serialparams.c_iflag |= (INPCK | ISTRIP);
    // serialparams.c_cc[VMIN] = 1;
    tcflush(handle, TCIFLUSH);
    tcsetattr(handle, TCSANOW, &serialparams);

    /* Allocate and clear serial structure.
     */
    myserial = (osalSerial*)os_malloc(sizeof(osalSerial), OS_NULL);
    if (myserial == OS_NULL)
    {
        rval = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        goto getout;
    }
    os_memclear(myserial, sizeof(osalSerial));

    /* Save serial handle and open flags.
     */
    myserial->handle = handle;
    myserial->open_flags = flags;

    /* Save interface pointer.
     */
    myserial->hdr.iface = &osal_serial_iface;

    /* Success set status code and cast serial structure pointer to stream pointer and return it.
     */
    if (status) *status = OSAL_SUCCESS;
    return (osalStream)myserial;

getout:
    /* If we got far enough to allocate the serial structure.
       Close the event handle (if any) and free memory allocated
       for the serial structure.
     */
    if (myserial)
    {
        os_free(myserial, sizeof(osalSerial));
    }

    /* Close serial
     */
    if (handle != -1)
    {
        close(handle);
    }

    /* Set status code and return NULL pointer.
     */
    if (status) *status = rval;
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Close serial port.
  @anchor osal_serial_close

  The osal_serial_close() function closes a serial port, earlier opened by the osal_serial_open()
  function. All resource related to the serial port are freed. Any attemp to use the serial after
  this call may result in crash.

  @param   stream Stream pointer representing the serial port. After this call stream
           pointer will point to invalid memory location.
  @return  None.

****************************************************************************************************
*/
void osal_serial_close(
	osalStream stream)
{
    osalSerial *myserial;

	/* If called with NULL argument, do nothing.
	 */
	if (stream == OS_NULL) return;

    /* Cast stream pointer to serial structure pointer. The osal_debug_assert here is used
       to detect access to already closed stream while debugging.
	 */
    myserial = (osalSerial*)stream;
    osal_debug_assert(myserial->hdr.iface == &osal_serial_iface);

    /* Close the serial port.
     */
    if (close(myserial->handle))
    {
        osal_debug_error("closeserial failed");
    }

#if OSAL_DEBUG
    myserial->hdr.iface = 0;
#endif

    /* Free memory allocated for the serial port structure.
     */
    os_free(myserial, sizeof(osalSerial));
}


/**
****************************************************************************************************

  @brief Flush data to the stream.
  @anchor osal_serial_flush

  Some implemntations of the osal_serial_flush() function flushes data to be written to stream
  or clear the transmit/receive buffers. The Linux implementation can clear RX and TX buffers.

  IMPORTANT, GENERALLY FLUSH MUST BE CALLED: The osal_stream_flush(<stream>, OSAL_STREAM_DEFAULT)
  must be called when select call returns even after writing or even if nothing was written, or
  periodically in in single thread mode. This is necessary even if no data was written
  previously, the stream may have stored buffered data to avoid blocking. This is not necessary
  for every stream implementation, but call it anyhow for code portability.

  @param   stream Stream pointer representing the serial port.
  @param   flags Bit fields. OSAL_STREAM_CLEAR_RECEIVE_BUFFER clears receive
           buffer and OSAL_STREAM_CLEAR_TRANSMIT_BUFFER transmit buffer.
           See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_serial_flush(
	osalStream stream,
	os_int flags)
{
    osalSerial *myserial;
    int qs;

    /* If called with NULL argument, do nothing.
     */
    if (stream == OS_NULL) return OSAL_STATUS_FAILED;

    /* Cast stream type to serial structure pointer, get operating system's serial port handle.
     */
    myserial = (osalSerial*)stream;
    osal_debug_assert(myserial->hdr.iface == &osal_serial_iface);

    if (flags & (OSAL_STREAM_CLEAR_RECEIVE_BUFFER|OSAL_STREAM_CLEAR_TRANSMIT_BUFFER))
    {
        switch (flags & (OSAL_STREAM_CLEAR_RECEIVE_BUFFER|OSAL_STREAM_CLEAR_TRANSMIT_BUFFER))
        {
            /* Flushes data received but not read.
             */
            case OSAL_STREAM_CLEAR_RECEIVE_BUFFER:
                qs = TCIFLUSH;
                break;

            /* Flushes data written but not transmitted.
             */
            case OSAL_STREAM_CLEAR_TRANSMIT_BUFFER:
                qs = TCOFLUSH;
                break;

            /* Flushes both data received but not read, and data written but not transmitted.
             */
            default:
                qs = TCIOFLUSH;
                break;
        }

        tcflush(myserial->handle, qs);
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Write data to serial port.
  @anchor osal_serial_write

  The osal_serial_write() function writes up to n bytes of data from buffer to serial port.

  @param   stream Stream pointer representing the serial port.
  @param   buf Pointer to the beginning of data to place into the serial port.
  @param   n Maximum number of bytes to write. 
  @param   n_written Pointer to integer into which the function stores the number of bytes 
           actually written to serial port, which may be less than n if there is not enough space
           left in write buffer. If the function fails n_written is set to zero.
  @param   flags Flags for the function, ignored. Set OSAL_STREAM_DEFAULT (0).
		   See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_serial_write(
	osalStream stream,
	const os_uchar *buf,
	os_memsz n,
	os_memsz *n_written,
	os_int flags)
{
    osalSerial *myserial;
    int rval, handle;

	if (stream)
	{
        /* Cast stream type to serial structure pointer, get operating system's serial port handle.
         */
        myserial = (osalSerial*)stream;
        osal_debug_assert(myserial->hdr.iface == &osal_serial_iface);
        handle = myserial->handle;

        /* If operating system serial is already closed.
         */
        if (buf == OS_NULL || n < 0)
        {
            goto getout;
        }

        /* Write data to serial port. Notice that linux handles the case when n is zero.
           If write to serial port returns an error, we do not really care, we treat it
           as zero bytes written.
         */
        rval = write(handle, buf, (int)n);
        if (rval < 0) rval = 0;

        /* We want select to check for wait only if we have unwritten data.
         */
        myserial->write_blocked = rval < n;

        /* Success, set number of bytes written.
         */
        *n_written = rval;
        return OSAL_SUCCESS;
	}

getout:
	*n_written = 0;
    return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Read data from serial port.
  @anchor osal_serial_read

  The osal_serial_read() function reads up to n bytes of data from serial port into buffer.

  @param   stream Stream pointer representing the serial port.
  @param   buf Pointer to buffer to read into.
  @param   n Maximum number of bytes to read. The data buffer must large enough to hold
           at least this many bytes.
  @param   n_read Pointer to integer into which the function stores the number of bytes read, 
           which may be less than n if there are fewer bytes available. If the function fails 
		   n_read is set to zero.
  @param   flags Flags for the function, ignored. Set OSAL_STREAM_DEFAULT (0).
		   See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_serial_read(
	osalStream stream,
	os_uchar *buf,
	os_memsz n,
	os_memsz *n_read,
	os_int flags)
{
    osalSerial *myserial;
    int handle, rval;

	if (stream)
	{
        /* Cast stream type to serial structure pointer, get operating system's serial
           port handle, check function argument.
		 */
        myserial = (osalSerial*)stream;
        osal_debug_assert(myserial->hdr.iface == &osal_serial_iface);
        handle = myserial->handle;
        if (buf == OS_NULL || n < 0) goto getout;

        /* Read from serial port. Notice that linux handles the case when n is zero.
           If read from serial port returns an error, we do not really care, we treat
           is as zero bytes received.
         */
        rval = read(handle, buf, (int)n);
        if (rval < 0) rval = 0;

        /* Success, set number of bytes read.
         */
        *n_read = rval;
		return OSAL_SUCCESS;
	}

getout:
    *n_read = 0;
    return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Get serial port parameter.
  @anchor osal_serial_get_parameter

  The osal_serial_get_parameter() function gets a parameter value. Here we just call the default
  implementation for streams.

  @param   stream Stream pointer representing the serial.
  @param   parameter_ix Index of parameter to get.
		   See @ref osalStreamParameterIx "stream parameter enumeration" for the list.
  @return  Parameter value.

****************************************************************************************************
*/
os_long osal_serial_get_parameter(
	osalStream stream,
	osalStreamParameterIx parameter_ix)
{
	return osal_stream_default_get_parameter(stream, parameter_ix);
}


/**
****************************************************************************************************

  @brief Set serial port parameter.
  @anchor osal_serial_set_parameter

  The osal_serial_set_parameter() function gets a parameter value. Here we just call the default
  implementation for streams.

  @param   stream Stream pointer representing the serial.
  @param   parameter_ix Index of parameter to get.
		   See @ref osalStreamParameterIx "stream parameter enumeration" for the list.
  @param   value Parameter value to set.
  @return  None.

****************************************************************************************************
*/
void osal_serial_set_parameter(
	osalStream stream,
	osalStreamParameterIx parameter_ix,
	os_long value)
{
	osal_stream_default_set_parameter(stream, parameter_ix, value);
}


#if OSAL_SERIAL_SELECT_SUPPORT
/**
****************************************************************************************************

  @brief Wait for an event from one of serials ports and for custom event.
  @anchor osal_serial_select

  The osal_serial_select() function blocks execution of the calling thread until something
  data is received from serial port, a last write has not been fully finished and it can
  be continued now, or a custom event occurs.

  Custom event and interrupting select: A pipe is generated for the event, and the select
  here monitors the pipe. When some other thread wants to interrupt the select() it
  calls oepal_set_event(), which write a byte to this pipe.

  @param   streams Array of streams to wait for. These must be serial ports, no mixing
           of different stream types is supported.
  @param   n_streams Number of stream pointers in "streams" array.
  @param   evnt Custom event to interrupt the select. OS_NULL if not needed.
  @param   selectdata Pointer to structure to fill in with information why select call
           returned. The "stream_nr" member is stream number which triggered the return,
           or OSAL_STREAM_NR_CUSTOM_EVENT if return was triggered by custom evenet given
           as argument. The "errorcode" member is OSAL_SUCCESS if all is fine. Other
           values indicate an error (broken or closed socket, etc). The "eventflags"
           member is planned to to show reason for return. So far value of eventflags
           is not well defined and is different for different operating systems, so
           it should not be relied on.
  @param   timeout_ms Maximum time to wait in select, ms. If zero, timeout is not used.
  @param   flags Ignored, set OSAL_STREAM_DEFAULT (0).

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_serial_select(
	osalStream *streams,
    os_int nstreams,
	osalEvent evnt,
	osalSelectData *selectdata,
    os_int timeout_ms,
	os_int flags)
{
    osalSerial *myserial;
    fd_set rdset, wrset;
    os_int i, handle, serial_nr, eventflags, errorcode, maxfd, pipefd, rval;
    struct timespec timeout, *to;
    
    os_memclear(selectdata, sizeof(osalSelectData));

    FD_ZERO(&rdset);
    FD_ZERO(&wrset);

    maxfd = 0;
    for (i = 0; i < nstreams; i++)
    {
        myserial = (osalSerial*)streams[i];
        if (myserial)
        {
            osal_debug_assert(myserial->hdr.iface == &osal_serial_iface);
            handle = myserial->handle;

            FD_SET(handle, &rdset);
            if (myserial->write_blocked)
            {
                FD_SET(handle, &wrset);
            }
            if (handle > maxfd) maxfd = handle;
        }
    }

    pipefd = -1;
    if (evnt)
    {
        pipefd = osal_event_pipefd(evnt);
        if (pipefd > maxfd) maxfd = pipefd;
        FD_SET(pipefd, &rdset);
    }

    to = NULL;
    if (timeout_ms)
    {
        timeout.tv_sec = (time_t)(timeout_ms / 1000);
        timeout.tv_nsec	= (long)((timeout_ms % 1000) * 1000000);
        to = &timeout;
    }

    errorcode = OSAL_SUCCESS;
    rval = pselect(maxfd+1, &rdset, &wrset, NULL, to, NULL);
    if (rval <= 0)
    {
        if (rval == 0)
        {
            selectdata->eventflags = OSAL_STREAM_TIMEOUT_EVENT;
            selectdata->stream_nr = OSAL_STREAM_NR_TIMEOUT_EVENT;
            return OSAL_SUCCESS;
        }
        errorcode = OSAL_STATUS_FAILED;
    }

    if (pipefd >= 0) if (FD_ISSET(pipefd, &rdset))
    {
        osal_event_clearpipe(evnt);

        selectdata->eventflags = OSAL_STREAM_CUSTOM_EVENT;
        selectdata->stream_nr = OSAL_STREAM_NR_CUSTOM_EVENT;
        return OSAL_SUCCESS;
    }

    eventflags = OSAL_STREAM_UNKNOWN_EVENT;

    for (serial_nr = 0; serial_nr < nstreams; serial_nr++)
    {
        myserial = (osalSerial*)streams[serial_nr];
        if (myserial)
        {
            handle = myserial->handle;

            if (FD_ISSET (handle, &rdset))
            {
                eventflags = OSAL_STREAM_READ_EVENT;
                break;
            }

            if (myserial->write_blocked)
            {
                if (FD_ISSET (handle, &wrset))
                {
                    eventflags = OSAL_STREAM_WRITE_EVENT;
                    break;
                }
            }
        }
    }

    if (serial_nr == nstreams)
    {
        serial_nr = OSAL_STREAM_NR_UNKNOWN_EVENT;
    }

    selectdata->eventflags = eventflags;
    selectdata->stream_nr = serial_nr;
    selectdata->errorcode = errorcode;

    return OSAL_SUCCESS;
}
#endif


/**
****************************************************************************************************

  @brief Get serial port name in format linux understands.
  @anchor osal_get_linux_serial_port_name

  The osal_get_linux_serial_port_name() gets serial port name from beginning of parameter
  string, converts it to format which linux understands (like "/dev/tty0" or "/dev/ttyUSB0")
  and moves parameter pointer to position where additional parameters may begin.

  Windows like COM port names COM1 ... COM4 in parameters string are interprented as
  "/dev/tty0", "/dev/tty1", "/dev/tty2" and "/dev/tty3" respectively. Similarly names
  COM5 ... COM8 correspond to "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyUSB2" and "/dev/ttyUSB3".

  If port name in parameters string isn't windows like COM port name, it is simply prefixed
  by "/dev/" and returned.

  @param   parameters Pointer to parameter string pointer. The "parameters" string should start
           with serial port name, like "tty0" or "COM10", the port name can be followed by optional
           colon, comma or semicolon to separate the rest of parameters (if any). The string
           pointer "parameters" is moved past port name.
  @param   portname Pointer to buffer where to store port name.
  @param   portname_sz Size of port name buffer in bytes.

  @return  None.

****************************************************************************************************
*/
static void osal_get_linux_serial_port_name(
    const os_char **parameters,
    os_char *portname,
    os_memsz portname_sz)
{
    os_char *d, *e, nbuf[OSAL_NBUF_SZ];
    const os_char *p;
    os_long com_nr;
    const os_int first_com_to_usb = 5;
    const os_char winport[] = "COM";
    const os_int winport_n = sizeof(winport) - 1;

    p = *parameters;
    e = portname + portname_sz - 1;
    while (osal_char_isspace(*p)) p++;

    os_strncpy(portname, "/dev/", portname_sz);

    if (!os_strnicmp(p, "COM", winport_n))
    {
        com_nr = osal_string_to_int(p + winport_n, OS_NULL);
        osal_int_to_string(nbuf, sizeof(nbuf),
            com_nr - (com_nr >= first_com_to_usb ? first_com_to_usb : 1));
        os_strncat(portname, com_nr >= first_com_to_usb ? "ttyUSB" : "tty", portname_sz);
        os_strncat(portname, nbuf, portname_sz);
        while (osal_char_isaplha(*p) || osal_char_isdigit(*p))
        {
            p++;
        }
    }
    else
    {
        d = os_strchr(portname, '\0');
        while (!osal_char_isspace(*p) && (osal_char_isaplha(*p) || osal_char_isdigit(*p)) && d < e)
        {
            *(d++) = *(p++);
        }
        *d = '\0';
    }

    while (osal_char_isspace(*p) || *p == ',' || *p == ';' || *p == ':')
    {
        p++;
    }

    *parameters = p;
}


/**
****************************************************************************************************

  @brief Initialize serial communication.
  @anchor osal_serial_initialize

  The osal_serial_initialize() initializes the underlying serial communication library.
  This is not needed for linux, just empty function to allow linking with code which
  calls this function for some other OS.

  @return  None.

****************************************************************************************************
*/
void osal_serial_initialize(
	void)
{
}


/**
****************************************************************************************************

  @brief Shut down the serial communication.
  @anchor osal_serial_shutdown

  The osal_serial_shutdown() shuts down the underlying serial communication library.
  This is not needed for linux, just empty function to allow linking with code which
  calls this function for some other OS.

  @return  None.

****************************************************************************************************
*/
void osal_serial_shutdown(
	void)
{
}


#if OSAL_FUNCTION_POINTER_SUPPORT

/** Stream interface for OSAL serials. This is structure osalStreamInterface filled with
    function pointers to OSAL serials implementation.
 */
osalStreamInterface osal_serial_iface
 = {osal_serial_open,
    osal_serial_close,
    osal_stream_default_accept,
    osal_serial_flush,
	osal_stream_default_seek,
    osal_serial_write,
    osal_serial_read,
	osal_stream_default_write_value,
	osal_stream_default_read_value,
    osal_serial_get_parameter,
    osal_serial_set_parameter,
#if OSAL_SERIAL_SELECT_SUPPORT
    osal_serial_select};
#else
    osal_stream_default_select};
#endif

#endif
#endif
