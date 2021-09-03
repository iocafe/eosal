/**

  @file    bluetooth/arduino/osal_bluetooth_esp32.cpp
  @brief   OSAL bluetooth API - Arduino implementation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Arduino bluetooth wrapper to provide OSAL stream interface.

  IMPORTANT
  We need RX buffer of 256 bytes.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#ifdef OSAL_ESP32
#if OSAL_BLUETOOTH_SUPPORT

#include <BluetoothSerial.h>

static BluetoothSerial SerialBT;
static osalStreamHeader bluetooth_handle;
static os_boolean bluetooth_initialized;

/**
****************************************************************************************************

  The osal_bluetooth_open() function opens a bluetooth port.

  Example:
    osalStream handle;
    handle = osal_bluetooth_open("MYESP32", OS_NULL, OS_NULL, OSAL_STREAM_NO_SELECT);
    if (handle == OS_NULL)
    {
        osal_debug_error("Unable to open bluetooth port");
        ...
    }

  @param  parameters Device name to display in bluetooth device discovery.
  @param  option Not used for bluetooth, set OS_NULL.

  @param  status Pointer to integer into which to store the function status code. Value
          OSAL_SUCCESS (0) indicates success and all nonzero values indicate an error.
          This parameter can be OS_NULL, if no status code is needed.

  @param  flags Flags for creating the bluetooth. Select is not supported in Arduino environment.

  @return Stream pointer representing the bluetooth port, or OS_NULL if the function failed.

****************************************************************************************************
*/
static osalStream osal_bluetooth_open(
    const os_char *parameters,
    void *option,
    osalStatus *status,
    os_int flags)
{
    if (!bluetooth_initialized)
    {
        SerialBT.begin(parameters);
        bluetooth_initialized = OS_TRUE;
    }

    if (status) *status = OSAL_SUCCESS;
    return &bluetooth_handle;
}


/**
****************************************************************************************************

  @brief Close bluetooth port.
  @anchor osal_bluetooth_close

  The osal_bluetooth_close() function closes a bluetooth port, earlier opened by the osal_bluetooth_open()
  function. All resource related to the bluetooth port are freed. Any attempt to use the bluetooth after
  this call may result in crash.

  @param   stream Stream pointer representing the bluetooth port. After this call stream
           pointer will point to invalid memory location.
  @return  None.

****************************************************************************************************
*/
static void osal_bluetooth_close(
    osalStream stream)
{
}


/**
****************************************************************************************************

  Some implementations of the osal_bluetooth_flush() function flushes data to be written to stream
  or clear the transmit/receive buffers. The Arguino implementation can clear RX and TX buffers.

  @param   stream Stream pointer representing the bluetooth port.
  @param   flags Bit fields. OSAL_STREAM_CLEAR_RECEIVE_BUFFER clears receive
           buffer. Clearing transmit buffer is not implemented for Arduino.
           See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error.

****************************************************************************************************
*/
static osalStatus osal_bluetooth_flush(
    osalStream stream,
    os_int flags)
{
    /* If called with NULL argument, do nothing.
     */
    if (stream == OS_NULL) return OSAL_STATUS_FAILED;

    if (flags & OSAL_STREAM_CLEAR_RECEIVE_BUFFER)
    {
        /* Clear the receive buffer.
         */
        while(SerialBT.available())
        {
            SerialBT.read();
        }
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Write data to bluetooth port.
  @anchor osal_bluetooth_write

  The osal_bluetooth_write() function writes up to n bytes of data from buffer to bluetooth port.

  @param   stream Stream pointer representing the bluetooth port.
  @param   buf Pointer to the beginning of data to place into the bluetooth port.
  @param   n Maximum number of bytes to write.
  @param   n_written Pointer to integer into which the function stores the number of bytes
           actually written to bluetooth port, which may be less than n if there is not enough space
           left in write buffer. If the function fails n_written is set to zero.
  @param   flags Flags for the function, ignored. Set OSAL_STREAM_DEFAULT (0).
           See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error.

****************************************************************************************************
*/
static osalStatus osal_bluetooth_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags)
{
    int nwr;

    if (stream)
    {
        /* See how much we have space in TX buffer. Write smaller number of bytes, either how
           many bytes can fit into TX buffer or buffer size n given as argument.
         */
        /* nwr = SerialBT.availableForWrite();
        if (n < nwr) nwr = n; */
        nwr = n;
        SerialBT.write((const uint8_t*)buf, nwr);

        /* Return number of bytes written.
         */
        *n_written = nwr;
        return OSAL_SUCCESS;
    }

    *n_written = 0;
    return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Read data from bluetooth port.
  @anchor osal_bluetooth_read

  The osal_bluetooth_read() function reads up to n bytes of data from bluetooth port into buffer.

  @param   stream Stream pointer representing the bluetooth port.
  @param   buf Pointer to buffer to read into.
  @param   n Maximum number of bytes to read. The data buffer must large enough to hold
           at least this many bytes.
  @param   n_read Pointer to integer into which the function stores the number of bytes read,
           which may be less than n if there are fewer bytes available. If the function fails
           n_read is set to zero.
  @param   flags Flags for the function, ignored. Set OSAL_STREAM_DEFAULT (0).
           See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error.

****************************************************************************************************
*/
static osalStatus osal_bluetooth_read(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags)
{
    int nrd;

    if (stream)
    {
        /* See how much data we in RX buffer. Read smaller number of bytes, either how
           many bytes we have in RX buffer or buffer size n given as argument.
         */
        nrd = SerialBT.available();
        if (n < nrd) nrd = n;
        SerialBT.readBytes(buf, nrd);

        /* Return number of bytes written.
         */
        *n_read = nrd;
        return OSAL_SUCCESS;
    }

    *n_read = 0;
    return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Initialize bluetooth communication.
  @anchor osal_bluetooth_initialize

  The osal_bluetooth_initialize() just makes sure that initialized flag is false.
  @return  None.

****************************************************************************************************
*/
void osal_bluetooth_initialize(
    void)
{
    bluetooth_initialized = OS_FALSE;
    os_global->bluetooth_shutdown_func = osal_bluetooth_shutdown;
}


/**
****************************************************************************************************

  @brief Shut down the bluetooth communication.
  @anchor osal_bluetooth_shutdown

  Called by osal_shutdown().

  The osal_bluetooth_shutdown() closes blue tooth, if it has been opened.
  @return  None.

****************************************************************************************************
*/
void osal_bluetooth_shutdown(
    void)
{
    if (bluetooth_initialized)
    {
        SerialBT.end();
        bluetooth_initialized = OS_FALSE;
    }
}


/** Stream interface for OSAL bluetooths. This is structure osalStreamInterface filled with
    function pointers to OSAL bluetooths implementation.
 */
const osalStreamInterface osal_bluetooth_iface
 = {OSAL_STREAM_IFLAG_NONE,
    osal_bluetooth_open,
    osal_bluetooth_close,
    osal_stream_default_accept,
    osal_bluetooth_flush,
    osal_stream_default_seek,
    osal_bluetooth_write,
    osal_bluetooth_read,
    osal_stream_default_select};

#endif
#endif
