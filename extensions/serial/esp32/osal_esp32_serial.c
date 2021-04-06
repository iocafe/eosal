/**

  @file    serial/arduino/osal_serial.cpp
  @brief   OSAL stream API implementation for Arduino serial communication.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

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
#if OSAL_SERIAL_SUPPORT

#include "driver/uart.h"

/** Arduino specific serial point state data structure. OSAL functions cast their own
    structure pointers to osalStream pointers.
 */
typedef struct osalSerial
{
    /** A stream structure must start with this generic stream header structure, which contains
        parameters common to every stream.
     */
    osalStreamHeader hdr;

    /**
     */
//    QueueHandle_t uart_queue;

    /** Stream open flags. Flags which were given to osal_serial_open() function.
     */
    os_int open_flags;
}
osalSerial;

#define OSAL_NRO_ESP32_UARTS UART_NUM_MAX
static osalSerial serialport[OSAL_NRO_ESP32_UARTS];


/* Prototypes for forward referred static functions.
 */
static uart_port_t osal_get_esp32_uart_nr(
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
          See @ref osalStatus "OSAL function return codes" for full list.
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
    uart_config_t uart_config;
    uart_port_t uart_nr;
    int rxbuf_sz, txbuf_sz;
    int rx_pin, tx_pin, rts_pin, cts_pin;
    const os_char *v;

    os_memclear(&uart_config, sizeof(uart_config));
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.stop_bits = UART_STOP_BITS_1;

    /* Get zero based port number and move parameters behind port name.
     */
    uart_nr = osal_get_esp32_uart_nr(&parameters);

    /* Baud rate.
     */
    uart_config.baud_rate = osal_str_get_item_int(parameters,
        "baud", 115200, OSAL_STRING_DEFAULT);

    /* Flow control.
     */
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    /* uart_config.flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS;
    uart_config.rx_flow_ctrl_thresh = 122; */

    /* Parity.
     */
    v = osal_str_get_item_value(parameters, "parity", OS_NULL, OSAL_STRING_DEFAULT);
    uart_config.parity = UART_PARITY_DISABLE;
    if (!os_strnicmp(v, "even", 4))
    {
        uart_config.parity = UART_PARITY_EVEN;
    }
    else if (!os_strnicmp(v, "odd", 3))
    {
        uart_config.parity = UART_PARITY_ODD;
    }

    /* Allocate and clear serial structure.
     */
    myserial = serialport + uart_nr;
    os_memclear((os_char*)myserial, sizeof(osalSerial));
    myserial->hdr.iface = OSAL_SERIAL_IFACE;
    myserial->open_flags = flags;

    /* Configure UART parameters.
     */
    ESP_ERROR_CHECK(uart_param_config(uart_nr, &uart_config));

    // ESP_ERROR_CHECK(uart_set_mode(uart_nr, UART_MODE_RS485_HALF_DUPLEX));

    /* Set UART pins. If unspecified, ESP32 defaults are used.
     */
    rx_pin = osal_str_get_item_int(parameters, "rxpin", UART_PIN_NO_CHANGE, OSAL_STRING_DEFAULT);
    tx_pin = osal_str_get_item_int(parameters, "txpin", UART_PIN_NO_CHANGE, OSAL_STRING_DEFAULT);
    rts_pin = osal_str_get_item_int(parameters, "rtspin", UART_PIN_NO_CHANGE, OSAL_STRING_DEFAULT);
    cts_pin = osal_str_get_item_int(parameters, "ctspin", UART_PIN_NO_CHANGE, OSAL_STRING_DEFAULT);
    uart_set_pin(uart_nr, tx_pin, rx_pin, rts_pin, cts_pin);

    /* Setup UART buffered IO with event queue.
     */
    rxbuf_sz = osal_str_get_item_int(parameters, "rxbuf", 256, OSAL_STRING_DEFAULT);
    txbuf_sz = osal_str_get_item_int(parameters, "txbuf", 256, OSAL_STRING_DEFAULT);
    if (rxbuf_sz < UART_FIFO_LEN + 16) rxbuf_sz = UART_FIFO_LEN + 16;
    if (txbuf_sz < UART_FIFO_LEN + 16) txbuf_sz = UART_FIFO_LEN + 16;
txbuf_sz = 0; // MUST BE ZERO, OTHERWISE BLOCKS
    ESP_ERROR_CHECK(uart_driver_install(uart_nr, rxbuf_sz, txbuf_sz,
        0, NULL, 0));
        //10, &myserial->uart_queue, 0));

    /* Configure the serial port.
     */
//     myserial->serial->begin(baudrate, port_config);

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
    uart_port_t uart_nr;

    /* Get UART number, If called with NULL argument, do nothing.
     */
    if (stream == OS_NULL) return;
    uart_nr = (osalSerial*)stream - serialport;

#if IDF_VERSION_MAJOR >= 4
    /* esp-idf version 4
     */
    if (uart_is_driver_installed(uart_nr))
    {
        uart_driver_delete(uart_nr);
    }
#else
    uart_driver_delete(uart_nr);
#endif
}


/**
****************************************************************************************************

  Some implementations of the osal_serial_flush() function flushes data to be written to stream
  or clear the transmit/receive buffers. The Arguino implementation can clear RX and TX buffers.

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
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_serial_flush(
    osalStream stream,
    os_int flags)
{
   uart_port_t uart_nr;

    /* Get UART number, If called with NULL argument, do nothing.
     */
    if (stream == OS_NULL) return OSAL_STATUS_FAILED;
    uart_nr = (osalSerial*)stream - serialport;

    if (flags & OSAL_STREAM_CLEAR_RECEIVE_BUFFER)
    {
        uart_flush_input(uart_nr);
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
    uart_port_t uart_nr;
    int nwr;

    /* Get UART number, If called with NULL argument, do nothing.
     */
    if (stream == OS_NULL) {
        *n_written = 0;
        return OSAL_STATUS_FAILED;
    }
    uart_nr = (osalSerial*)stream - serialport;

    nwr = uart_tx_chars(uart_nr, buf, n);

    if (nwr < 0)
    {
        *n_written = 0;
        return OSAL_STATUS_FAILED;
    }

    /* Return number of bytes written.
     */
    *n_written = nwr;
    osal_resource_monitor_update(OSAL_RMON_TX_SERIAL, nwr);
    return OSAL_SUCCESS;
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
    uart_port_t uart_nr;
    size_t nrd, available;

    /* Get UART number, If called with NULL argument, do nothing.
     */
    if (stream == OS_NULL) {
        *n_read = 0;
        return OSAL_STATUS_FAILED;
    }
    uart_nr = (osalSerial*)stream - serialport;

    ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_nr, &available));
    if (available <= 0) {
        nrd = 0;
    }
    else {
        if (n > available) n = available;
        nrd = uart_read_bytes(uart_nr, (uint8_t*)buf, (uint32_t)n, 0);
        osal_resource_monitor_update(OSAL_RMON_RX_SERIAL, nrd);
    }

    /* Return number of bytes written.
     */
    *n_read = nrd;
    return OSAL_SUCCESS;
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
    /* Call the default implementation
     */
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
    /* Call the default implementation
     */
    osal_stream_default_set_parameter(stream, parameter_ix, value);
}


/**
****************************************************************************************************

  @brief Get serial port number.
  @anchor osal_get_esp32_uart_nr

  The osal_get_esp32_uart_nr() gets zero based UART number and moves parameters pointer past
  com port selection to other serial paramerers:
  - Windows like COM port name COM1 ... COM4 in is converted to UART0 .. UART 4.

  @param   parameters Pointer to parameter string pointer. The "parameters" string should start
           with serial port name, like "COM2", the port name can be followed by optional
           colon, comma or semicolon to separate the rest of parameters (if any). The string
           pointer "parameters" is moved past port name.

  @return  Serial port number 0...

****************************************************************************************************
*/
static uart_port_t osal_get_esp32_uart_nr(
    const os_char **parameters)
{
    const os_char *p;
    uart_port_t uart_nr;
    const os_char winport[] = "COM";
    const os_int winport_n = sizeof(winport) - 1;

    p = *parameters;
    while (osal_char_isspace(*p)) p++;

    /* Get the first integer number (single digit) which we encouner in uart_nr.
     */
    uart_nr = 0;
    while (osal_char_isalpha(*p) || osal_char_isdigit(*p)) {
        if (osal_char_isdigit(*p)) uart_nr = *p - '1';
        p++;
    }

    /* If this is windows specific marking, port number start from 1, decrement.
     */
    if (!os_strnicmp(p, winport, winport_n)) {
        uart_nr--;
    }

    /* Skip until end end of serial port selection in parameter string.
     */
    while (osal_char_isspace(*p) || *p == ',' || *p == ';' || *p == ':') {
        p++;
    }

    /* Make sure that UART number is legimate.
     */
    if (uart_nr < 0 || uart_nr >= OSAL_NRO_ESP32_UARTS)
    {
        uart_nr = 0;
    }

    /* Move parameter pointer past end of serial port selection and return UART number.
     */
    *parameters = p;
    return uart_nr;
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
    /* osal_global->serial_shutdown_func = osal_serial_shutdown; */
}


/**
****************************************************************************************************

  @brief Shut down the serial communication.
  @anchor osal_serial_shutdown

  The osal_serial_shutdown() shuts down the underlying serial communication library.
  This is not needed for Arduino, just empty function to allow linking with code which
  calls this function for some other OS.

  @return  None.

****************************************************************************************************
static void osal_serial_shutdown(
    void)
{
}
*/


#if OSAL_MINIMALISTIC == 0
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
    osal_stream_default_write_value,
    osal_stream_default_read_value,
    osal_serial_get_parameter,
    osal_serial_set_parameter,
    osal_stream_default_select};
#endif

#endif
