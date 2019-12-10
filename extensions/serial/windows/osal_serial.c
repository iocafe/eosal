/**

  @file    serial/windows/osal_serial.c
  @brief   OSAL stream API implementation for windows serial communication.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    15.11.2019

  Serial communication. Implementation of OSAL stream API for Windows serial ports.

  Windows overlapped IO is used to monitor received data and avaliablility of transmit buffer.
  On top of this API windows serial port can be used with select much like a non blocking serial 
  port in linux. 

  Configuring a Communications Resource:
  https://msdn.microsoft.com/en-us/library/windows/desktop/aa363201(v=vs.85).aspx

  Overlapped serial communcation:
  https://docs.microsoft.com/en-us/previous-versions/ff802693(v=msdn.10)

  Copyright 2012 Pekka Lehtikoski. This file is part of the eobjects project and shall only 
  be used, modified, and distributed under the terms of the project licensing. By continuing 
  to use, modify, or distribute this file you indicate that you have read the license and 
  understand and accept it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_SERIAL_SUPPORT
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>


/** Windows specific serial point state data structure. OSAL functions cast their own
    structure pointers to osalStream pointers.
 */
typedef struct osalSerial
{
    /** A stream structure must start with this generic stream header structure, which contains
        parameters common to every stream.
     */
    osalStreamHeader hdr;

    /** Operating system's serial port handle.
     */
    HANDLE h;

#if OSAL_SERIAL_SELECT_SUPPORT
    /** Windows overlapped IO structure.
     */
    OVERLAPPED ov;

    /** Flag to mark that we are already monitoring communication status
     */
    os_boolean monitoring_status;

    /** Communication status from WaitCommState()
     */
    DWORD status_event;

    /** Event to do reads and writes
     */
    HANDLE rw_event;
#endif

    /** Stream open flags. Flags which were given to osal_serial_open() function.
     */
    os_int open_flags;
} 
osalSerial;


/* Prototypes for forward referred static functions.
 */
static void osal_get_windows_serial_port_name(
    const os_char **parameters,
    os_char *portname,
    os_memsz portname_sz);

static void osal_serial_monitor_status(
    osalSerial *myserial);


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

  @param  parameters Serial port name and parameters, for example "COM5,baud=115200".
          The "parameters" string must beging with serial port name.
          The port name can be followed by com port settings, in format "name=value".
          These settings are separated from serial port name and other setting by comma.
          Currectly supported settings are baud=<baudrate> and parity=none/odd/even.

  @param  parameters Serial parameters, a list string or direct value.
          Address and port to connect to, or interface and port to
          listen for. Serial IP address and port can be specified either as value of
          "addr" item or directly in parameter sstring. For example "192.168.1.55:20" or
          "localhost:12345" specify IPv4 addressed. If only port number is specified,
          which is often useful for listening serial, for example ":12345". IPv4 address
          is automatically recognized from numeric address like
          "2001:0db8:85a3:0000:0000:8a2e:0370:7334", but not when address is
          specified as string nor for empty IP specifying only port to listen. Use
          brackets around IP address to mark IPv6 address, for example
          "[localhost]:12345", or "[]:12345" for empty IP.

  @param  option Not used for serial ports, set OS_NULL.

  @param  status Pointer to integer into which to store the function status
          code. Value OSAL_SUCCESS (0) indicates success and all nonzero values indicate
          an error. See @ref osalStatus "OSAL function return codes" for full list. This
          parameter can be OS_NULL, if no status code is needed.

  @param  flags Flags for creating the serial. Bit fields, combination of:
          - OSAL_STREAM_NO_SELECT: Open serial without select functionality.
          - OSAL_STREAM_SELECT: Open serial with select functionality (default).
          See @ref osalStreamFlags "Flags for Stream Functions" for full list of stream flags.

  @return Stream pointer representing the serial, or OS_NULL if the function failed.

****************************************************************************************************
*/
osalStream osal_serial_open(
    const os_char *parameters,
    void *option,
    osalStatus *status, 
    os_int flags) 
{
    HANDLE h = INVALID_HANDLE_VALUE;
    DCB dcb;
    COMMTIMEOUTS timeouts;
    osalStatus s = OSAL_STATUS_FAILED;
    osalSerial *myserial = OS_NULL;
    os_char portname[32];
    const os_char *v;
    os_ushort wportname[32];
    os_boolean use_select;

    /* Get serial port name, convert it to windows format and then to UTF16
    */
    osal_get_windows_serial_port_name(&parameters, portname, sizeof(portname));
    osal_str_utf8_to_utf16(
        wportname, sizeof(wportname) / sizeof(os_ushort), portname);

    use_select = (os_boolean)((flags & (OSAL_STREAM_NO_SELECT|OSAL_STREAM_SELECT)) == OSAL_STREAM_SELECT);

    /* Open the serial port
     */
    h = CreateFileW(wportname,
       GENERIC_READ|GENERIC_WRITE,  // access ( read and write)
       0,                           // (share) 0:cannot share the COM port
       0,                           // security  (None)
       OPEN_EXISTING,               // creation : open_existing
#if OSAL_SERIAL_SELECT_SUPPORT
       use_select ? FILE_FLAG_OVERLAPPED : 0,        // we want overlapped operation
#else
       0,
#endif
       0);                          // no templates file for COM port...

    if (h == INVALID_HANDLE_VALUE)
    {
        goto getout;
    }

    os_memclear(&dcb, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);
 
    if (!GetCommState(h, &dcb))
    {
        osal_debug_error("GetCommState failed");
        goto getout;
    }
 
    /* Baud rate.
     */
    dcb.BaudRate = (DWORD)osal_str_get_item_int(parameters,
        "baud", 115200, OSAL_STRING_DEFAULT);

    /* Parity.
     */
    v = osal_str_get_item_value(parameters,
        "parity", OS_NULL, OSAL_STRING_DEFAULT);
    dcb.Parity = NOPARITY;
    if (!os_strnicmp(v, "even", 4))
    {
        dcb.Parity = EVENPARITY;
    }
    else if (!os_strnicmp(v, "odd", 3))
    {
        dcb.Parity = ODDPARITY;
    }

    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    // dcb.fOutxCtsFlow
  
    if (!SetCommState(h, &dcb))
    {
        osal_debug_error("SetCommState failed");
        goto getout;
    }

    /* Set zero timeouts.
     */
    os_memclear(&timeouts, sizeof(timeouts));
    timeouts.ReadIntervalTimeout                = MAXDWORD;
    if (!SetCommTimeouts(h, &timeouts))
    {
        osal_debug_error("SetCommState failed");
        goto getout;
    }

    // SetCommBuffers?

#if OSAL_SERIAL_SELECT_SUPPORT
    if (use_select)
    {
        SetCommMask(h, EV_TXEMPTY | EV_RXCHAR);
    }
#endif

    /* Allocate and set up serial structure.
     */
    myserial = (osalSerial*)os_malloc(sizeof(osalSerial), OS_NULL);
    if (myserial == OS_NULL)
    {
        s = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        goto getout;
    }
    
    os_memclear(myserial, sizeof(osalSerial));
    myserial->hdr.iface = &osal_serial_iface;
    myserial->open_flags = flags;
    myserial->h = h;

#if OSAL_SERIAL_SELECT_SUPPORT
    if (use_select)
    {
        myserial->ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        myserial->rw_event = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (myserial->ov.hEvent == NULL || myserial->rw_event == NULL)
        {
            osal_debug_error("osal_serial:CreateEvent failed");
            goto getout;
        }
        
        osal_serial_monitor_status(myserial);
    }
#endif

    if (status) *status = OSAL_SUCCESS;
    return (osalStream)myserial;

getout:
    if (status) *status = s;
    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    os_free(myserial, sizeof(osalSerial));
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
    osalStream stream,
    os_int flags)
{
    osalSerial *myserial;
    HANDLE h;

    /* If called with NULL argument, do nothing.
    */
    if (stream == OS_NULL)
        return;

    /* Cast stream pointer to serial structure pointer and get operating system's serial port handle.
    */
    myserial = (osalSerial *)stream;
    osal_debug_assert(myserial->hdr.iface == &osal_serial_iface);
    h = myserial->h;

    /* Mark serial port closed.
     */
    myserial->h = INVALID_HANDLE_VALUE;

    /* Close the serial port.
     */
    if (!CloseHandle(h))
    {
        osal_debug_error("CloseHandle failed");
    }

#if OSAL_SERIAL_SELECT_SUPPORT
    if (myserial->ov.hEvent)
    {
        if (!CloseHandle(myserial->ov.hEvent))
        {
            osal_debug_error("CloseHandle failed");
        }
    }
    if (myserial->rw_event)
    {
        if (!CloseHandle(myserial->rw_event))
        {
            osal_debug_error("CloseHandle failed");
        }
    }
#endif

#if OSAL_DEBUG
    myserial->hdr.iface = 0;
#endif

    /* Free memory allocated for serial structure.
    */
    os_free(myserial, sizeof(osalSerial));
}


/**
****************************************************************************************************

  @brief Flush data to the stream.
  @anchor osal_serial_flush

  Some implementations of the osal_serial_flush() function flushes data to be written to stream.
  Currently the implementation for Linux serial port does nothing.

  IMPORTANT, GENERALLY FLUSH MUST BE CALLED: The osal_stream_flush(<stream>, OSAL_STREAM_DEFAULT)
  must be called when select call returns even after writing or even if nothing was written, or
  periodically in in single thread mode. This is necessary even if no data was written
  previously, the stream may have stored buffered data to avoid blocking. This is not necessary
  for every stream implementation, but call it anyhow for code portability.

  @param   stream Stream pointer representing the serial port.
  @param   flags See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_serial_flush(
    osalStream stream, 
    os_int flags) 
{
    osalSerial *myserial;
    DWORD  dwFlags;
    BOOL ok;

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
                dwFlags = PURGE_RXCLEAR;
                break;

            /* Flushes data written but not transmitted.
             */
            case OSAL_STREAM_CLEAR_TRANSMIT_BUFFER:
                dwFlags = PURGE_TXCLEAR;
                break;

            /* Flushes both data received but not read, and data written but not transmitted.
             */
            default:
                dwFlags = PURGE_RXCLEAR|PURGE_TXCLEAR;
                break;
        }

        ok = PurgeComm(myserial->h, dwFlags);
        if (!ok)
        {
            osal_debug_error("PurgeComm failed");
            return OSAL_STATUS_FAILED;
        }
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
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written, 
    os_int flags) 
{
    DWORD nwr;
    osalSerial *myserial;
    HANDLE h;
    osalStatus status = OSAL_STATUS_FAILED;
    int err;

#if OSAL_SERIAL_SELECT_SUPPORT
    OVERLAPPED ov;
#endif

    if (stream) 
    {
        /* Cast stream pointer to serial structure pointer.
            */
        myserial = (osalSerial *)stream;
        osal_debug_assert(myserial->hdr.iface == &osal_serial_iface);

        /* Special case. Writing 0 bytes will trigger write callback by worker thread.
         */
        if (n == 0) 
        {
            *n_written = 0;
            return OSAL_SUCCESS;
        }

        /* get operating system's serial port handle. Fail, if operating system serial port is already closed.
         */
        h = myserial->h;

#if OSAL_SERIAL_SELECT_SUPPORT
        if ((myserial->open_flags & (OSAL_STREAM_NO_SELECT|OSAL_STREAM_SELECT)) == OSAL_STREAM_SELECT)
        {
            os_memclear(&ov, sizeof(ov));
            ov.hEvent = myserial->rw_event;
            ResetEvent(ov.hEvent);
            
            if (!WriteFile(h, buf, (DWORD)n, &nwr, &ov)) 
            {
                err = GetLastError();
                if (err != ERROR_IO_PENDING) 
                {
                    goto getout;
                }

                /* Wait for the overlapped write to complete.
                 */
  			    switch (WaitForSingleObject(ov.hEvent, INFINITE))
  			    {
  			        case WAIT_OBJECT_0:
  				        /* The overlapped operation has succesfully completed.
                         */
  				        if (!GetOverlappedResult(h, &ov, &nwr, FALSE))
  				        {
  					        osal_debug_error("osal_serial.c,write: no overlapped result");
  					        return OSAL_STATUS_FAILED;
  				        }
  				        break;
  
  			        case WAIT_TIMEOUT:
  				        /* The operation timed out, cancel the I/O operation. 
                         */
  				        CancelIo(h);
  				        return OSAL_STATUS_TIMEOUT;
  
  			        default:
  				        /* Failed, just quit.
                         */
  				        osal_debug_error("osal_serial.c,write: Wait failed?");
  				        return OSAL_STATUS_FAILED;
                }
            }
        }
        else 
        {
            if (!WriteFile(h, buf, (DWORD)n, &nwr, NULL)) 
            {
                err = GetLastError();
                if (err != ERROR_IO_PENDING) 
                {
                    goto getout;
                }
            }
        }
#else
        if (!WriteFile(h, buf, (DWORD)n, &nwr, NULL)) 
        {
            err = GetLastError();
            if (err != ERROR_IO_PENDING) 
            {
                goto getout;
            }
        }
#endif

        *n_written = nwr;
        return OSAL_SUCCESS;
    }

getout:
    *n_written = 0;
    return status;
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
    os_char *buf,
    os_memsz n,
    os_memsz *n_read, 
    os_int flags) 
{
    osalSerial *myserial;
    DWORD nr;
    HANDLE h;
    osalStatus status = OSAL_STATUS_FAILED;

#if OSAL_SERIAL_SELECT_SUPPORT
    int err;
    OVERLAPPED ov;
#endif

    if (stream) 
    {
        /* Cast stream pointer to serial structure pointer, lock serial and get OS
           serial handle.
         */
        myserial = (osalSerial *)stream;
        osal_debug_assert(myserial->hdr.iface == &osal_serial_iface);
        h = myserial->h;

#if OSAL_SERIAL_SELECT_SUPPORT
        if ((myserial->open_flags & (OSAL_STREAM_NO_SELECT|OSAL_STREAM_SELECT)) == OSAL_STREAM_SELECT)
        {
            os_memclear(&ov, sizeof(ov));
            ov.hEvent = myserial->rw_event;
            ResetEvent(ov.hEvent);
            
            if (!ReadFile(h, buf, (DWORD)n, &nr, &ov)) 
            {
                err = GetLastError();
                if (err != ERROR_IO_PENDING) 
                {
                    goto getout;
                }

                /* Wait for the overlapped write to complete.
                 */
  			    switch (WaitForSingleObject(ov.hEvent, INFINITE))
  			    {
  			        case WAIT_OBJECT_0:
  				        /* The overlapped operation has succesfully completed.
                         */
  				        if (!GetOverlappedResult(h, &ov, &nr, FALSE))
  				        {
  					        osal_debug_error("osal_serial.c,read: no overlapped result");
  					        return OSAL_STATUS_FAILED;
  				        }
  				        break;
  
  			        case WAIT_TIMEOUT:
  				        /* The operation timed out, cancel the I/O operation. 
                         */
  				        CancelIo(h);
  				        return OSAL_STATUS_TIMEOUT;
  
  			        default:
  				        /* Failed, just quit.
                         */
  				        osal_debug_error("osal_serial.c,read: Wait failed?");
  				        return OSAL_STATUS_FAILED;
                }
            }
        }
        else 
        {
            if (!ReadFile(h, buf, (DWORD)n, &nr, NULL)) 
            {
                err = GetLastError();
                if (err != ERROR_IO_PENDING) 
                {
                    goto getout;
                }
            }
        }
#else
        if (!ReadFile(h, buf, (DWORD)n, &nr, NULL)) 
        {
            int err = GetLastError();
            if (err != ERROR_IO_PENDING) 
            {
                goto getout;
            }
        } 
#endif

        *n_read = nr;
        return OSAL_SUCCESS;
    }
    status;

getout:
    *n_read = 0;
    return status;
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

  @param   streams Array of streams to wait for. These must be sockets, no mixing
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
    HANDLE events[OSAL_SERIAL_SELECT_MAX+1];
    DWORD dwWait;
    DWORD dwOvRes;
    static DWORD dwEventMask = 0;
    os_int i, n_serials, n_events; 

    os_memclear(selectdata, sizeof(osalSelectData));

    if (nstreams < 1 || nstreams > OSAL_SERIAL_SELECT_MAX)
        return OSAL_STATUS_FAILED;

    n_serials = 0;
    for (i = 0; i < nstreams; i++)
    {
        myserial = (osalSerial*)streams[i];
        if (myserial)
        {
            osal_debug_assert(myserial->hdr.iface == &osal_serial_iface);
            events[n_serials] = myserial->ov.hEvent;
            n_serials++;

            osal_serial_monitor_status(myserial);
        }
    }
    n_events = n_serials;

    /* If we have event, add it to wait.
     */
    if (evnt)
    {
        events[n_events++] = evnt;
    }
    
    dwWait = WaitForMultipleObjects (n_events, events, FALSE, timeout_ms ? timeout_ms : INFINITE);

    if (dwWait >= WAIT_OBJECT_0 && dwWait < WAIT_OBJECT_0 + n_serials)
    {
        myserial = (osalSerial*)streams[dwWait - WAIT_OBJECT_0];
        GetOverlappedResult(myserial->h, &myserial->ov, &dwOvRes, FALSE);

        if ( myserial->status_event & EV_TXEMPTY )
        {
            selectdata->eventflags |= OSAL_STREAM_WRITE_EVENT;
            osal_trace3("EV_TXEMPTY");
        }

        if ( myserial->status_event & EV_RXCHAR)
        {
            selectdata->eventflags |= OSAL_STREAM_READ_EVENT;
            osal_trace3("EV_RXCHAR");
        }

         myserial->monitoring_status = OS_FALSE;
         osal_serial_monitor_status(myserial);

         selectdata->stream_nr = (dwWait - WAIT_OBJECT_0);
    }
    else  if (dwWait == WAIT_OBJECT_0 + n_serials)
    {
        selectdata->eventflags = OSAL_STREAM_CUSTOM_EVENT;
        selectdata->stream_nr = OSAL_STREAM_NR_CUSTOM_EVENT;
    }
    else if (dwWait == WAIT_TIMEOUT)
    {
        selectdata->eventflags = OSAL_STREAM_TIMEOUT_EVENT;
        selectdata->stream_nr = OSAL_STREAM_NR_TIMEOUT_EVENT;
        return OSAL_SUCCESS;
    }
    else
    {
        selectdata->eventflags = OSAL_STREAM_UNKNOWN_EVENT;
        selectdata->stream_nr = OSAL_STREAM_NR_UNKNOWN_EVENT;
    }


    return OSAL_SUCCESS;
}
#endif


#if OSAL_SERIAL_SELECT_SUPPORT
/**
****************************************************************************************************

  @brief Initialize serial communication.
  @anchor osal_serial_monitor_status

  The osal_serial_monitor_status() function sets up monitoring of serial port events, unless
  it is set up already.

  @param   myserial Pointer to serial structure.
  @return  None.

****************************************************************************************************
*/
static void osal_serial_monitor_status(
    osalSerial *myserial)
{
    if (!myserial->monitoring_status) 
    {
        myserial->status_event = 0;

        if (!WaitCommEvent(myserial->h, &myserial->status_event, &myserial->ov)) 
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
               myserial->monitoring_status = OS_TRUE;
            }
            else
            {
                myserial->monitoring_status = OS_TRUE;
                osal_debug_error("WaitCommEvent() failed to monitor status");
                return;
            }
        }
        else
        {
            /* We got the event immediately
             */
            SetEvent(myserial->ov.hEvent);
        }
    }
}
#endif


/**
****************************************************************************************************

  @brief Get serial port name in format windows understands.
  @anchor osal_get_windows_serial_port_name

  The osal_get_windows_serial_port_name() gets serial port name from beginning of parameter 
  string, converts it for format windows understands (like "\\\\.\\COM10") and moves parameter 
  pointer to position where additional parameters may begin. That format with backslashes is
  required for COM ports, where number is bigger than 9, but should work for all the COM ports.

  @param   parameters Pointer to parameter string pointer. The parameters string should
           start with COM port name, like "COM10", the port name can be followed by optional
           colon, comma or semicolon to separate the rest of parameters (if any). The string
           pointer "parameters" is moved past port name.
  @param   portname Pointer to buffer where to store port name.
  @param   portname_sz Size of port name buffer in bytes.

  @return  None.

****************************************************************************************************
*/
static void osal_get_windows_serial_port_name(
    const os_char **parameters,
    os_char *portname,
    os_memsz portname_sz) 
{
    os_char *d, *e;
    const os_char *p;

    p = *parameters;
    e = portname + portname_sz - 1;
    while (osal_char_isspace(*p)) p++;
    os_strncpy(portname, "\\\\.\\", portname_sz);
    d = os_strchr(portname, '\0');

    while (!osal_char_isspace(*p) && (osal_char_isaplha(*p) || osal_char_isdigit(*p)) && d < e) 
    {
        *(d++) = *(p++);
    }
    *d = '\0';

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
  This is not needed for Windows, just empty function to allow linking with code which
  calls this function for some other OS.

  @return  None.

****************************************************************************************************
*/
void osal_serial_initialize(void) 
{
}


/**
****************************************************************************************************

  @brief Shut down the serial communication.
  @anchor osal_serial_shutdown

  The osal_serial_shutdown() shuts down the underlying serial communication library.
  This is not needed for Windows, just empty function to allow linking with code which
  calls this function for some other OS.

  @return  None.

****************************************************************************************************
*/
void osal_serial_shutdown(void) 
{
}

#if OSAL_FUNCTION_POINTER_SUPPORT

const osalStreamInterface osal_serial_iface
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
