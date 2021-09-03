/**

  @file    serial/arduino/osal_serial.cpp
  @brief   OSAL stream API implementation for Arduino serial communication.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Serial communication. Implementation of OSAL stream API for Arduino serial ports.

  IMPORTANT
  We need RX buffer of 256 bytes and SERIAL_RX_BUFFER_SIZE is 64 by default.
  This is used HardwareSerial.h.

  Create file named "build_opt.h" in Arduino sketch folder. The file should contain only text:
  -DSERIAL_RX_BUFFER_SIZE=256 -DSERIAL_TX_BUFFER_SIZE=256

  IMPORTANT: STM32DUINO CANNOT RELIABLY HANDLE BUFFER SIZES GREATER THAN 256 BYTES.
  RARE CORRUPTION OF MESSAGE RESULTS FROM TRYING THIS. PERHAPS SAME IN OTHER ARDUINO
  BASED SYSTEMS, BUT THIS IS NOT PROVEN.

  Does this really do anything?
  stty -F /dev/ttyUSB0 -ixon

  To display
  stty -a -F /dev/ttyUSB0

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#ifdef OSAL_ARDUINO
#if OSAL_SERIAL_SUPPORT
#if OSAL_SERIAL_SUPPORT == OSAL_DUINO_SERIAL_NORMAL

#include <HardwareSerial.h>

  /** Arduino specific serial point state data structure. OSAL functions cast their own
      structure pointers to osalStream pointers.
   */
  typedef struct osalSerial
  {
    /** A stream structure must start with this generic stream header structure, which contains
        parameters common to every stream.
     */
    osalStreamHeader hdr;

    /** Pointer to global serial port object.
     */
    HardwareSerial *serial;
  }
  osalSerial;

  #define OSAL_NRO_ARDUINO_SERIAL_PORTS 3
  static osalSerial serialport[OSAL_NRO_ARDUINO_SERIAL_PORTS];

  #ifdef USART1
    static HardwareSerial myUART1(USART1);
  #endif

  #ifdef USART2
    static HardwareSerial myUART2(USART2);
  #endif

  #ifdef USART3
    #define PIN_SERIAL3_TX PC4
    #define PIN_SERIAL3_RX PC5
    static HardwareSerial myUART3(USART3);
  #endif

  #ifdef USART4
    static HardwareSerial myUART4(USART4);
    #define MYUART4
  #else
    #ifdef UART4
      static HardwareSerial myUART4(UART4);
      #define MYUART4
    #endif
  #endif


  /* Prototypes for forward referred static functions.
   */
  static int osal_get_arduino_serial_port_nr(
      const os_char **parameters);

/**
****************************************************************************************************

  The osal_serial_open() function opens a serial port.

  Example:
    osalStream handle;
    handle = osal_serial_open("COM2,baud=38400", OS_NULL, OS_NULL, OSAL_STREAM_NO_SELECT);
    if (handle == OS_NULL)
    {
        osal_debug_error("Unable to open serial port");
        ...
    }

  @param  parameters Serial port name and parameters, for example "COM2,baud=38400".
          The parameters string must beging with serial port name. It is Windows
          like COMx port name, "COM1" means Arduino Serial object, "COM2" Arduino Serial1
          object...
          The port name can be followed by com port settings, in format "name=value".
          These settings are separated from serial port name and other setting by comma.
          Currectly supported settings are baud=<baudrate> and parity=none/odd/even.

  @param  option Not used for serial ports, set OS_NULL.

  @param  status Pointer to integer into which to store the function status code. Value
          OSAL_SUCCESS (0) indicates success and all nonzero values indicate an error.
          This parameter can be OS_NULL, if no status code is needed.

  @param  flags Flags for creating the serial. Bit fields, combination of:
          - OSAL_STREAM_NO_SELECT: Open serial without select functionality. Use this
          always for Arduino. Select is not supported in Arduino environment.

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
    const os_char *v;
    long baudrate;
    int port_config, portnr0;

    /* Get zero based port number and move parameters behind port name.
     */
    portnr0 = osal_get_arduino_serial_port_nr(&parameters);

    /* Baud rate.
     */
    baudrate = (long)osal_str_get_item_int(parameters, "baud", 115200, OSAL_STRING_DEFAULT);

    /* Parity.
     */
    v = osal_str_get_item_value(parameters, "parity", OS_NULL, OSAL_STRING_DEFAULT);
    port_config = SERIAL_8N1;
    if (!os_strnicmp(v, "even", 4))
    {
        port_config = SERIAL_8E1;
    }
    else if (!os_strnicmp(v, "odd", 3))
    {
        port_config = SERIAL_8O1;
    }

    /* Allocate and clear serial structure.
     */
    myserial = serialport + portnr0;
    os_memclear(myserial, sizeof(osalSerial));
    myserial->hdr.iface = OSAL_SERIAL_IFACE;

    /* Set up serial port structure.
     */
    switch (portnr0)
    {
        default:

#ifdef USART1
        case 0:
            myserial->serial = &myUART1;
            break;

#endif

#ifdef USART2
        case 1:
            myserial->serial = &myUART2;
            break;
#endif

#ifdef USART3
        case 2:
#ifdef PIN_SERIAL3_TX
            myUART3.setTx(PIN_SERIAL3_TX);
            myUART3.setRx(PIN_SERIAL3_RX);
#endif
            myserial->serial = &myUART3;
            break;
#endif

#ifdef MYUART4
        case 3:
            myserial->serial = &myUART4;
            break;
#endif
        break;
    }

    /* Configure the serial port.
     */
    myserial->serial->begin(baudrate, port_config);

    /* Success set status code and cast serial structure pointer to stream pointer and return it.
     */
    if (status) *status = OSAL_SUCCESS;
    return (osalStream)myserial;
}

/**
****************************************************************************************************

  @brief Close serial port.
  @anchor osal_serial_close

  The osal_serial_close() function closes a serial port, earlier opened by the osal_serial_open()
  function. All resource related to the serial port are freed. Any attempt to use the serial after
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

    /* If called with NULL argument, do nothing.
     */
    if (stream == OS_NULL) return;

    /* Cast stream pointer to serial port structure pointer.
     */
    myserial = (osalSerial*)stream;

    /* Finish with serial communication.
     */
    myserial->serial->end();
}


/**
****************************************************************************************************

  Some implementations of the osal_serial_flush() function flushes data to be written to stream
  or clear the transmit/receive buffers. The Arduino implementation can clear RX and TX buffers.

  IMPORTANT, GENERALLY FLUSH MUST BE CALLED: The osal_stream_flush(<stream>, OSAL_STREAM_DEFAULT)
  must be called when select call returns even after writing or even if nothing was written, or
  periodically in in single thread mode. This is necessary even if no data was written
  previously, the stream may have stored buffered data to avoid blocking. This is not necessary
  for every stream implementation, but call it anyhow for code portability.

  @param   stream Stream pointer representing the serial port.
  @param   flags Bit fields. OSAL_STREAM_CLEAR_RECEIVE_BUFFER clears receive
           buffer. Clearing transmit buffer is not implemented for Arduino.
           See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error.

****************************************************************************************************
*/
osalStatus osal_serial_flush(
    osalStream stream,
    os_int flags)
{
    osalSerial *myserial;

    /* If called with NULL argument, do nothing.
     */
    if (stream == OS_NULL) return OSAL_STATUS_FAILED;

    /* Cast stream type to serial structure pointer, get operating system's serial port handle.
     */
    myserial = (osalSerial*)stream;
    osal_debug_assert(myserial->hdr.iface == OSAL_SERIAL_IFACE);

    if (flags & OSAL_STREAM_CLEAR_RECEIVE_BUFFER)
    {
        /* Clear the receive buffer.
         */
        while(myserial->serial->available())
        {
            myserial->serial->read();
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
           indicate an error.

****************************************************************************************************
*/
osalStatus osal_serial_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags)
{
    osalSerial *myserial;
    int nwr;

    if (stream)
    {
        /* Cast stream pointer to serial port structure pointer.
         */
        myserial = (osalSerial*)stream;

        /* See how much we have space in TX buffer. Write smaller number of bytes, either how
           many bytes can fit into TX buffer or buffer size n given as argument.
         */
        nwr = myserial->serial->availableForWrite();
        if (n < nwr) nwr = n;
        myserial->serial->write((const uint8_t*)buf, nwr);

        /* Return number of bytes written.
         */
        *n_written = nwr;
        osal_resource_monitor_update(OSAL_RMON_TX_SERIAL, nwr);
        return OSAL_SUCCESS;
    }

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
           indicate an error.

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
    int nrd;

    if (stream)
    {
        /* Cast stream pointer to serial port structure pointer.
         */
        myserial = (osalSerial*)stream;

        /* See how much data we in RX buffer. Read smaller number of bytes, either how
           many bytes we have in RX buffer or buffer size n given as argument.
         */
        nrd = myserial->serial->available();
        if (n < nrd) nrd = n;
        myserial->serial->readBytes(buf, nrd);

        /* Return number of bytes written.
         */
        *n_read = nrd;
        osal_resource_monitor_update(OSAL_RMON_RX_SERIAL, nrd);
        return OSAL_SUCCESS;
    }

    *n_read = 0;
    return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Get serial port number.
  @anchor osal_get_arduino_serial_port_nr

  The osal_get_arduino_serial_port_nr() gets zero based com port number from Windows like COM
  port name COM1 ... COM4 in beginning of the parameters string.

  @param   parameters Pointer to parameter string pointer. The "parameters" string should start
           with serial port name, like "COM2", the port name can be followed by optional
           colon, comma or semicolon to separate the rest of parameters (if any). The string
           pointer "parameters" is moved past port name.

  @return  Serial port number 0...

****************************************************************************************************
*/
static int osal_get_arduino_serial_port_nr(
    const os_char **parameters)
{
    const os_char *p;
    int portnr0;

    p = *parameters;
    while (osal_char_isspace(*p)) p++;

    portnr0 = 0;
    while (osal_char_isalpha(*p) || osal_char_isdigit(*p))
    {
        if (osal_char_isdigit(*p)) portnr0 = *p - '1';
        p++;
    }

    while (osal_char_isspace(*p) || *p == ',' || *p == ';' || *p == ':')
    {
        p++;
    }

    *parameters = p;
    if (portnr0 < 0 || portnr0 >= OSAL_NRO_ARDUINO_SERIAL_PORTS)
    {
        portnr0 = 0;
    }
    return portnr0;
}


/**
****************************************************************************************************

  @brief Initialize serial communication.
  @anchor osal_serial_initialize

  The osal_serial_initialize() initializes the underlying serial communication library.
  This is not needed for Arduino, just empty function to allow linking with code which
  calls this function for some other OS.

  @return  None.

****************************************************************************************************
*/
void osal_serial_initialize(
    void)
{
}


/** Stream interface for OSAL serials. This is structure osalStreamInterface filled with
    function pointers to OSAL serials implementation.
 */
OS_CONST osalStreamInterface osal_serial_iface
 = {OSAL_STREAM_IFLAG_NONE,
    osal_serial_open,
    osal_serial_close,
    osal_stream_default_accept,
    osal_serial_flush,
    osal_stream_default_seek,
    osal_serial_write,
    osal_serial_read,
    osal_stream_default_select};

#endif
#endif
#endif
