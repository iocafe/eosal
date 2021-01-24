/**

  @file    serial/metal/osal_serial.c
  @brief   OSAL stream API implementation for some STM32 core M4 chips.
  @brief   OSAL serial port metal implementation.
  @author  Pekka Lehtikoski
  @date    8.1.2020

  Serial communication. Implementation of OSAL stream for some STM32 core M4 chips.

  THIS CODE NEEDS TO MOVE TO CHIP SPECIFIC FILE, like osal_serial_stm32_a.c and be more
  clearly enabled/disabled by chip type define.

  NOTES:
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
#define OSAL_INCLUDE_METAL_HEADERS
#include "eosalx.h"

#if OSAL_SERIAL_SUPPORT

/* STM32L4 Low level headers.
 * USE_FULL_LL_DRIVER must be defined in compiler settings for cube generated code
 */
#ifndef USE_FULL_LL_DRIVER
#define USE_FULL_LL_DRIVER
#endif

#ifdef STM32L476xx
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_rcc.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_usart.h"
#else
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_usart.h"
#endif

struct osalSerial;
typedef void osal_serial_func(void);

/* A structure to hold serial port hardware details, like USART instance, interrupt,
   clock enable functions and IO pins.
 */
typedef struct
{
    int com_port_nr;
    USART_TypeDef *instance;
    int irq; // IRQn_Type *irq;
    osal_serial_func *gpio_clk_enable;
    osal_serial_func *clk_enable;
    osal_serial_func *clk_source;

    GPIO_TypeDef *tx_gpio_port;
    uint32_t tx_pin;
    osal_serial_func *set_tx_gpio_af;

    GPIO_TypeDef *rx_gpio_port;
    uint32_t rx_pin;
    osal_serial_func *set_rx_gpio_af;

    GPIO_TypeDef *tx_ctrl_gpio_port;
    uint32_t tx_ctrl_pin;

    struct osalSerial *serial;
}
osalStaticUARTConfig;

/* Configuration functions for USART 3
 */
static void osal_uart3_gpio_clk_enable(void)
{
#ifndef STM32F429xx
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
#endif
}
static void osal_uart3_clk_enable(void)
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);
}
static void osal_uart3_clk_source(void)
{
#ifndef STM32F429xx
    LL_RCC_SetUSARTClockSource(LL_RCC_USART3_CLKSOURCE_PCLK1);
#endif
}
static void osal_uart3_set_tx_gpio_af(void)
{
    LL_GPIO_SetAFPin_0_7(GPIOC, LL_GPIO_PIN_4, LL_GPIO_AF_7);
}
static void osal_uart3_set_rx_gpio_af(void)
{
    LL_GPIO_SetAFPin_0_7(GPIOC, LL_GPIO_PIN_5, LL_GPIO_AF_7);
}


/* Static hardware information for all USARTs which can be accessed through this wrapper.
 */
static osalStaticUARTConfig osal_uart[] = {
    {3, /* Use marking COM3 */
     USART3,
     USART3_IRQn,

     osal_uart3_gpio_clk_enable,
     osal_uart3_clk_enable,
     osal_uart3_clk_source,

     GPIOC,
     LL_GPIO_PIN_4,
     osal_uart3_set_tx_gpio_af,

     GPIOC,
     LL_GPIO_PIN_5,
     osal_uart3_set_rx_gpio_af,

     GPIOC,
     LL_GPIO_PIN_1},
};

/* Number of USARTs known to this wrapper.
 */
#define OSAL_NRO_UARTS (sizeof(osal_uart)/sizeof(osalStaticUARTConfig))


/* Ring buffer size, same for both receive and transmit.
 */
#define OSAL_SERIAL_RING_BUF_SZ 256

/* Maximum number of open serial ports.
 */
#define OSAL_MAX_OPEN_SERIAL_PORTS 2

/** Serial point state data structure. OSAL functions cast their own structure pointers
    to osalStream pointers.
 */
typedef struct osalSerial
{
    /** A stream structure must start with this generic stream header structure, which contains
        parameters common to every stream.
     */
    osalStreamHeader hdr;

    /** Pointer to global usart confifuration.
     */
    osalStaticUARTConfig *uart;

    /* Ring buffers.
     */
    os_uchar
        txbuf[OSAL_SERIAL_RING_BUF_SZ],
        rxbuf[OSAL_SERIAL_RING_BUF_SZ],
        *txend,
        *rxend;

    /* Ring buffer heads and tails.
     */
    volatile os_uchar
        * volatile txhead,
        * volatile rxhead,
        * volatile txtail,
        * volatile rxtail;

    __IO uint8_t
        sendflag;

    /** Stream open flags. Flags which were given to osal_serial_open() function.
     */
    os_int open_flags;

}
osalSerial;

/* Reserve memory for serial ports.
 */
static osalSerial osal_serial_port[OSAL_MAX_OPEN_SERIAL_PORTS];


/* Forward referred static functions.
 */
static int osal_get_metal_serial_port_nr(
    const os_char **parameters);

static void osal_serial_irq_handler(
    osalStaticUARTConfig *uart);


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
          like COMx port name, "COM1" means first known UART, "COM2" second known
          UART...
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
    LL_USART_InitTypeDef
        initstruc;

    osalStaticUARTConfig
        *uart;

    osalSerial
        *myserial;

    const os_char
        *v;

    int
        port_nr,
        i;

    /* Clear parameter structure.
     */
    os_memclear(&initstruc, sizeof(initstruc));

    /* Decide on UART configuration structure and serial port structure.
     */
    port_nr = osal_get_metal_serial_port_nr(&parameters);
    for (i = 0; osal_uart[i].com_port_nr != port_nr; i++)
    {
        if (i >= OSAL_NRO_UARTS - 1)
        {
            /* No UART matches to port number.
             */
            if (status) *status = OSAL_SUCCESS;
            return OS_NULL;
        }
    }
    uart = osal_uart + i;
    myserial = uart->serial;
    if (myserial == OS_NULL)
    {
        for (i = 0; osal_serial_port[i].uart; i++)
        {
            if (i >= OSAL_MAX_OPEN_SERIAL_PORTS - 1)
            {
                /* No free serial port structures.
                 */
                if (status) *status = OSAL_SUCCESS;
                return OS_NULL;

            }
        }
        myserial = osal_serial_port + i;
    }
    myserial->uart = uart;
    myserial->hdr.iface = OSAL_SERIAL_IFACE;
    myserial->open_flags = flags;
    uart->serial = myserial;

    /* Parse baud rate "bps". Default to 115200.
     */
    initstruc.BaudRate = osal_str_get_item_int(parameters, "baud",
        115200, OSAL_STRING_DEFAULT);

    /* Number of bits in byte is always 8.
     */
    initstruc.DataWidth = LL_USART_DATAWIDTH_8B;

    /* Get parity check option: "none", "odd" or "even". Defaults to "none".
     */
    v = osal_str_get_item_value(parameters, "parity", OS_NULL, OSAL_STRING_DEFAULT);
    initstruc.Parity = LL_USART_PARITY_NONE;
    if (!os_strnicmp(v, "even", 4))
    {
        initstruc.Parity = LL_USART_PARITY_EVEN;
    }
    if (!os_strnicmp(v, "odd", 3))
    {
        initstruc.Parity = LL_USART_PARITY_ODD;
    }

    /* One stop bit.
     */
    initstruc.StopBits = LL_USART_STOPBITS_1;

    /* Set up the rest.
     */
    initstruc.TransferDirection   = LL_USART_DIRECTION_TX_RX;
    initstruc.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    initstruc.OverSampling        = LL_USART_OVERSAMPLING_16;

    /* Enable the peripheral clock of GPIO Port.
     */
    uart->gpio_clk_enable();

    /* Configure Tx Pin as : Alternate function, High Speed, Push pull, Pull up.
     */
    LL_GPIO_SetPinMode(uart->tx_gpio_port, uart->tx_pin, LL_GPIO_MODE_ALTERNATE);
    uart->set_tx_gpio_af();
    LL_GPIO_SetPinSpeed(uart->tx_gpio_port, uart->tx_pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinOutputType(uart->tx_gpio_port, uart->tx_pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(uart->tx_gpio_port, uart->tx_pin, LL_GPIO_PULL_UP);

    /* Configure Rx Pin as : Alternate function, High Speed, Push pull, Pull up.
     */
    LL_GPIO_SetPinMode(uart->rx_gpio_port, uart->rx_pin, LL_GPIO_MODE_ALTERNATE);
    uart->set_rx_gpio_af();
    LL_GPIO_SetPinSpeed(uart->rx_gpio_port, uart->rx_pin, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinOutputType(uart->rx_gpio_port, uart->rx_pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(uart->rx_gpio_port, uart->rx_pin, LL_GPIO_PULL_UP);

    /* Transmitter control pin configuration
     */
    LL_GPIO_SetPinMode(uart->tx_ctrl_gpio_port, uart->tx_ctrl_pin, LL_GPIO_MODE_OUTPUT);

    /* Set serial interrupt priority and enable the interrupt.
     */
    NVIC_SetPriority(uart->irq, 0);
    NVIC_EnableIRQ(uart->irq);

    /* Enable USART clock and clock source.
     */
    uart->clk_enable();
    uart->clk_source();

    /* Setup empty ring buffers
     */
    myserial->txend = myserial->txbuf + OSAL_SERIAL_RING_BUF_SZ;
    myserial->rxend = myserial->rxbuf + OSAL_SERIAL_RING_BUF_SZ;
    myserial->txhead = myserial->txtail = myserial->txbuf;
    myserial->rxhead = myserial->rxtail = myserial->rxbuf;
    myserial->sendflag = OS_FALSE;

    /* Disable transmitter.
     */
#ifndef STM32F429xx
    HAL_GPIO_WritePin(uart->tx_ctrl_gpio_port, uart->tx_ctrl_pin, GPIO_PIN_RESET);
#endif

    /* Initialize UART using the initialization structure.
     */
    LL_USART_Init(uart->instance, &initstruc);

    /* Enable the UART and wait for initialisation.
     */
    LL_USART_Enable(uart->instance);
    while((!(LL_USART_IsActiveFlag_TEACK(uart->instance))) ||
          (!(LL_USART_IsActiveFlag_REACK(uart->instance))))
    {
    }

    /* Enable RXNE interrupt.
     */
    LL_USART_EnableIT_RXNE(uart->instance);

    /* Success.
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
    osalStaticUARTConfig *uart;
    osalSerial *myserial;

    if (stream)
    {
        myserial =  (osalSerial*)stream;
        uart = myserial->uart;

        LL_USART_DisableIT_RXNE(uart->instance);
        LL_USART_DisableIT_TXE(uart->instance);
        LL_USART_DisableIT_TC(uart->instance);
        LL_USART_Disable(uart->instance);

        uart->serial = OS_NULL;
        myserial->uart = OS_NULL;
    }
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
           buffer and OSAL_STREAM_CLEAR_TRANSMIT_BUFFER the tranmit buffer.
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

    /* If called with NULL argument, do nothing.
     */
    if (stream == OS_NULL) return OSAL_STATUS_FAILED;

    myserial = (osalSerial*)stream;

    if (flags & OSAL_STREAM_CLEAR_RECEIVE_BUFFER)
    {
        myserial->rxhead = myserial->rxtail = myserial->rxbuf;
    }

    if (flags & OSAL_STREAM_CLEAR_TRANSMIT_BUFFER)
    {
        myserial->rxhead = myserial->rxtail = myserial->rxbuf;
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
    volatile os_uchar *next, *tail;
    __IO os_uchar c;
    os_uchar *end;
    int bytes;
    osalSerial *myserial;
    osalStaticUARTConfig *uart;

    /* It is error to write to closed stream.
     */
    if (stream == OS_NULL)
    {
        *n_written = 0;
        return OSAL_STATUS_FAILED;
    }

    myserial = (osalSerial*)stream;
    uart = myserial->uart;

    /* Write to the ring buffer.
     */
    bytes = 0;
    end = myserial->txend;
    tail = myserial->txtail;
    while (n > 0)
    {
        next = myserial->txhead + 1;
        if (next == end) next = myserial->txbuf;
        if (next == tail)
        {
            /* Ring buffer overflow.
             */
            break;
        }

        *myserial->txhead = *(buf++);
        myserial->txhead = next;
        n--;
        bytes++;
    }

    /* Start transfer only if not already running.
     */
    if (!myserial->sendflag && myserial->txhead != tail)
    {
        /* Enable transmitter.
         */
#ifndef STM32F429xx
        HAL_GPIO_WritePin(uart->tx_ctrl_gpio_port, uart->tx_ctrl_pin, GPIO_PIN_SET);
#endif

        /* Fill TDR with a new char.
         */
        c = *tail;
        if (++tail == end) tail = myserial->txbuf;
        myserial->txtail = tail;
        myserial->sendflag = OS_TRUE;

        /* Start USART transmission : Will initiate TXE interrupt after
           TDR register is empty.
         */
        LL_USART_TransmitData8(uart->instance, c);

        /* Enable TXE interrupt.
         */
        LL_USART_EnableIT_TXE(uart->instance);
    }

    *n_written = bytes;
    osal_resource_monitor_update(OSAL_RMON_TX_SERIAL, bytes);
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
    osalSerial *myserial;
    os_uchar *end;
    int bytes;

    /* It is error to read closed stream.
     */
    if (stream == OS_NULL)
    {
        *n_read = 0;
        return OSAL_STATUS_FAILED;;
    }
    myserial =  (osalSerial*)stream;

    /* Read up to n characters from the ring buffer.
     */
    bytes = 0;
    end = myserial->rxend;
    while (n > 0 && myserial->rxhead != myserial->rxtail)
    {
        *(buf++) = *myserial->rxtail;
        n--;
        bytes++;
        if (++(myserial->rxtail) == end) myserial->rxtail = myserial->rxbuf;
    }

    *n_read = bytes;
    osal_resource_monitor_update(OSAL_RMON_RX_SERIAL, bytes);
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
    /* Call the default implementation.
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
    /* Call the default implementation.
     */
    osal_stream_default_set_parameter(stream, parameter_ix, value);
}


/**
****************************************************************************************************

  @brief Get serial port number.

  The osal_get_metal_serial_port_nr() gets one based com port number from Windows like COM
  port name COM1 ... COM4 in beginning of the parameters string.

  @param   parameters Pointer to parameter string pointer. The "parameters" string should start
           with serial port name, like "COM2", the port name can be followed by optional
           colon, comma or semicolon to separate the rest of parameters (if any). The string
           pointer "parameters" is moved past port name.

  @return  Serial port number 1...

****************************************************************************************************
*/
static int osal_get_metal_serial_port_nr(
    const os_char **parameters)
{
    const os_char *p;
    int portnr;

    p = *parameters;
    while (osal_char_isspace(*p)) p++;

    portnr = 1;
    while (osal_char_isaplha(*p) || osal_char_isdigit(*p))
    {
        if (osal_char_isdigit(*p)) portnr = *p - '0';
        p++;
    }

    while (osal_char_isspace(*p) || *p == ',' || *p == ';' || *p == ':')
    {
        p++;
    }

    *parameters = p;
    return portnr;
}


/**
****************************************************************************************************

  @brief Interrupt handler, receive character.

  The osal_serial_irq_receive_char function reads a received character from chip and places
  it to receive ring buffer.
  @return None.

****************************************************************************************************
*/
static void osal_serial_irq_receive_char(
    osalStaticUARTConfig *uart)
{
    __IO uint32_t c;
    volatile unsigned char *next;
    osalSerial *myserial;

    myserial = uart->serial;

    /* Read a received character. RXNE flag is cleared by reading the RDR register.
     */
    c = LL_USART_ReceiveData8(uart->instance);

    next = myserial->rxhead + 1;
    if (next == myserial->rxend)
    {
        next = myserial->rxbuf;
    }
    if (next == myserial->rxtail)
    {
        /* Receive ring buffer overflow.
         */
        return;
    }

    *(myserial->rxhead) = (unsigned char)c;
    myserial->rxhead = next;
}


/**
****************************************************************************************************

  @brief Interrupt: handler send character.

  The osal_serial_irq_send_char function moves a character from send ring buffer to the UART chip.
  If there is no character to send, the function disables TXE interrupt and enables TC interrupt.
  @return None.

****************************************************************************************************
*/
static void osal_serial_irq_send_char(
    osalStaticUARTConfig *uart)
{
    __IO unsigned char c;
    osalSerial *myserial;

    myserial = uart->serial;

    /* If noting to send, then disable TXE interrupt and enable TC interrupt.
     */
    if (myserial->txhead == myserial->txtail)
    {
        LL_USART_DisableIT_TXE(uart->instance);
        myserial->sendflag = OS_FALSE;
        LL_USART_EnableIT_TC(uart->instance);
        return;
    }

    /* Move char from ring buffer to serial chip.
     */
    c = *myserial->txtail;
    if (++(myserial->txtail) == myserial->txend) myserial->txtail = myserial->txbuf;
    LL_USART_TransmitData8(uart->instance, c);
}


/**
****************************************************************************************************

  @brief Interrupt: handler send character.

  The osal_serial_irq_is_complete_check function is called once last byte has been completely
  transmitted. This is intended for transmitter control.
  @return None.

****************************************************************************************************
*/
static void osal_serial_irq_is_complete_check(
    osalStaticUARTConfig *uart)
{
    osalSerial *myserial;
    myserial = uart->serial;

    if (!myserial->sendflag)
    {
        LL_USART_DisableIT_TC(uart->instance);
#ifndef STM32F429xx
        HAL_GPIO_WritePin(uart->tx_ctrl_gpio_port, uart->tx_ctrl_pin, GPIO_PIN_RESET);
#endif
    }
}


/**
****************************************************************************************************

  @brief Serial interrupt handler.

  The osal_serial_irq_handler function is handles serial UART chip specific interrup handling.
  Call this function interrupt handler generated by the Cube.
  @return None.

****************************************************************************************************
*/
void osal_serial_uart3_irq_handler(void)
{
    osal_serial_irq_handler(&osal_uart[0]);
}


/**
****************************************************************************************************

  @brief Serial interrupt handler.

  The osal_serial_irq_handler function is handles serial UART chip specific interrup handling.
  Call this function interrupt handler generated by the Cube.
  @return None.

****************************************************************************************************
*/
static void osal_serial_irq_handler(
    osalStaticUARTConfig *uart)
{
    if (uart->serial == OS_NULL) return;

    /* Check if we have received a character. If character has been received, RXNE flag will
       be cleared by RDR register read.
     */
    if(LL_USART_IsActiveFlag_RXNE(uart->instance) &&
       LL_USART_IsEnabledIT_RXNE(uart->instance))
    {
        osal_serial_irq_receive_char(uart);
    }

    /* Check if UART has space new character to send. The TXE flag will be cleared when writing
       new data in TDR register.
     */
    if(LL_USART_IsEnabledIT_TXE(uart->instance) &&
       LL_USART_IsActiveFlag_TXE(uart->instance))
    {
        osal_serial_irq_send_char(uart);
    }

    /* Has all data stored in UART been sent? If so, clear TC flag and prepare to
       disable the transmitter.
     */
    if(LL_USART_IsEnabledIT_TC(uart->instance) &&
       LL_USART_IsActiveFlag_TC(uart->instance))
    {
        LL_USART_ClearFlag_TC(uart->instance);
        osal_serial_irq_is_complete_check(uart);
    }
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
    int i;

    for (i = 0; i < OSAL_NRO_UARTS; i++)
    {
        osal_uart[i].serial = OS_NULL;
    }

    for (i = 0; i < OSAL_MAX_OPEN_SERIAL_PORTS; i++)
    {
        osal_serial_port[i].uart = OS_NULL;
    }
}


#if OSAL_PROCESS_CLEANUP_SUPPORT
/**
****************************************************************************************************

  @brief Shut down the serial communication.
  @anchor osal_serial_shutdown

  The osal_serial_shutdown() shuts down the underlying serial communication library.
  This is not needed for Arduino, just empty function to allow linking with code which
  calls this function for some other OS.

  @return  None.

****************************************************************************************************
*/
void osal_serial_shutdown(
    void)
{
}
#endif


#if OSAL_MINIMALISTIC == 0
/** Stream interface for OSAL serials. This is structure osalStreamInterface filled with
    function pointers to OSAL serials implementation.
 */
OS_FLASH_MEM osalStreamInterface osal_serial_iface
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
