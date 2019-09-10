/**

  @file    socket/metal/osal_socket_w5500.c
  @brief   OSAL sockets for bare metal with WizNet library.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    20.1.2017

  Implementation of OSAL sockets for W5500 bare metal using Wiznet library.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#define OSAL_INCLUDE_METAL_HEADERS
#include "eosalx.h"
#if OSAL_SOCKET_SUPPORT

#include "wizchip_conf.h"
#include "W5500/w5500.h"
#include "socket.h"
#include "../Internet/DHCP/dhcp.h"

// WHAT TO DO ABOUT THIS ???????????????????????????????????????????
extern SPI_HandleTypeDef hspi1;


/* Default network configuration strings. Locally administered MAC address ranges safe
   for testing: x2:xx:xx:xx:xx:xx, x6:xx:xx:xx:xx:xx, xA:xx:xx:xx:xx:xx and xE:xx:xx:xx:xx:xx
 */
#define OSAL_IP_ADDRESS_DEFAULT "192.168.1.201"
#define OSAL_SUBNET_MASK_DEFAULT "255.255.255.0"
#define OSAL_GATEWAY_ADDRESS_DEFAULT "192.168.1.254"
#define OSAL_DNS_ADDRESS_DEFAULT "8.8.8.8"
#define OSAL_MAC_DEFAULT "6A-7F-18-67-A1-D3"

/** Global network setup. Micro-controllers typically have one (or two)
    network interfaces. The network interface configuration is managed
    here, not by operating system.
 */
osalNetworkInterface osal_net_iface
  = {"METAL",                       /* host_name */
     OSAL_IP_ADDRESS_DEFAULT,       /* ip_address */
     OSAL_SUBNET_MASK_DEFAULT,      /* subnet_mask */
     OSAL_GATEWAY_ADDRESS_DEFAULT,  /* gateway_address */
     OSAL_DNS_ADDRESS_DEFAULT,      /* dns_address */
     OSAL_MAC_DEFAULT,              /* mac */
     0};                            /* dhcp */

/** Socket library initialized flag.
 */
os_boolean osal_sockets_initialized = OS_FALSE;

/* Initialization flags used by this module.
 */
static os_boolean osal_w5500_chip_initialized = OS_FALSE;
static os_boolean osal_network_configured = OS_FALSE;

/** Socket number, etc defines.
 */
#define OSAL_MAX_SOCKETS 8          /* Maximum number of sockets. */
#define OSAL_ALL_USED 127           /* Number to mark no available socket */
#define OSAL_NRO_W5500_PORTS 8      /* Number of ports on w5500 chip */

/** Possible socket uses.
 */
typedef enum
{
    OSAL_SOCKET_UNUSED = 0,
    OSAL_SOCKET_CLIENT,
    OSAL_SOCKET_SERVER,
    OSAL_SOCKET_UDP,
    OSAL_SOCKET_DHCP
}
osalSocketUse;

/** Possible socket states.
 */
typedef enum
{
    OSAL_SOCKET_NOT_CONFIGURED = 0,
    OSAL_SOCKET_CONNECTED,
}
osalSocketState;

/** WizNET W5500, etc specific socket structure to store information.
 */
typedef struct osalSocket
{
    /** A stream structure must start with this generic stream header structure, which contains
        parameters common to every stream.
     */
    osalStreamHeader hdr;

    /** Nonzero if socket is used. Zero (OSAL_SOCKET_UNUSED) indicates if not used.
        See enumeration osalSocketUse.
     */
    os_uchar use;

    /** Socket state. See enumeration osalSocketState.
     */
    os_uchar state;

    /** Wiznet chip's socket port index, 0 - 7.
     */
    os_uchar port_ix;

    /** IP address given as parameter to osal_socket_open().
     */
    os_uchar ip_address[4];

    /** TCP or UDP port number given as parameter to osal_socket_open().
     */
    os_ushort port_nr;

    /** Local TCP or UDP port number.
     */
    os_ushort local_port_nr;
}
osalSocket;

/* Array of socket structures for every possible WizNet sockindex
 */
static osalSocket osal_socket[OSAL_MAX_SOCKETS];


/* Prototypes for forward referred static functions.
 */
static os_short osal_get_unused_socket(
    void);

static os_short osal_get_unused_w5500_port(
    void);

static void osal_ip_from_str(
    uint8_t *ip,
    const os_char *str);

static void osal_mac_from_str(
    uint8_t mac[6],
    const char* str);

static void osal_initialize_wiz_chip(
    void);

static void osal_setup_network(
    void);

static void osal_start_dhcp(
    void);

static void osal_make_sockets(
    void);


/**
****************************************************************************************************

  @brief Open a socket.
  @anchor osal_socket_open

  The osal_socket_open() function opens a socket. The socket can be either listening TCP 
  socket, connecting TCP socket or UDP multicast socket. 

  @param  parameters Socket parameters, a list string or direct value.
		  Address and port to connect to, or interface and port to listen for.
          Socket IP address and port can be specified either as value of "addr" item
          or directly in parameter sstring. For example "192.168.1.55:20" or "localhost:12345"
          specify IPv4 addressed. If only port number is specified, which is often 
          useful for listening socket, for example ":12345".
          IPv4 address is automatically recognized from numeric address like
          "2001:0db8:85a3:0000:0000:8a2e:0370:7334", but not when address is specified as string
          nor for empty IP specifying only port to listen. Use brackets around IP address
          to mark IPv6 address, for example "[localhost]:12345", or "[]:12345" for empty IP.

  @param  option Not used for sockets, set OS_NULL.

  @param  status Pointer to integer into which to store the function status code. Value
		  OSAL_SUCCESS (0) indicates success and all nonzero values indicate an error.
          See @ref osalStatus "OSAL function return codes" for full list.
		  This parameter can be OS_NULL, if no status code is needed. 

  @param  flags Flags for creating the socket. Bit fields, combination of:
          - OSAL_STREAM_CONNECT: Connect to specified socket port at specified IP address. 
          - OSAL_STREAM_LISTEN: Open a socket to listen for incoming connections. 
          - OSAL_STREAM_UDP_MULTICAST: Open a UDP multicast socket. 
          - OSAL_STREAM_NO_SELECT: Open socket without select functionality.
          - OSAL_STREAM_SELECT: Open socket with select functionality.
          - OSAL_STREAM_TCP_NODELAY: Disable Nagle's algorithm on TCP socket.
          - OSAL_STREAM_NO_REUSEADDR: Disable reusability of the socket descriptor.
          - OSAL_STREAM_BLOCKING: Open socket in blocking mode.

		  See @ref osalStreamFlags "Flags for Stream Functions" for full list of stream flags.

  @return Stream pointer representing the socket, or OS_NULL if the function failed.

****************************************************************************************************
*/
osalStream osal_socket_open(
    const os_char *parameters,
	void *option,
	osalStatus *status,
	os_int flags)
{
    os_int port_nr;
    os_char host[OSAL_HOST_BUF_SZ];
    os_short mysocket_ix;
    os_boolean is_ipv6;
    osalSocket *mysocket;
    osalStatus rval = OSAL_STATUS_FAILED;

    /* Initialize sockets library, if not already initialized.
     */
    if (!osal_sockets_initialized)
    {
        osal_socket_initialize();
    }

    /* Initialize Wiznet w5500 chip and set the MAC address.
     */
    if (!osal_w5500_chip_initialized)
    {
        osal_initialize_wiz_chip();
    }

    /* Setup IP address and other network parameters.
     */
    if (!osal_network_configured)
    {
        osal_setup_network();
    }

	/* Get host name or numeric IP address and TCP port number from parameters.
       The host buffer must be released by calling os_free() function,
       unless if host is OS_NULL (unpecified).
	 */
    port_nr = OSAL_DEFAULT_SOCKET_PORT;
    osal_socket_get_host_name_and_port(parameters,
        &port_nr, host, sizeof(host), &is_ipv6);

    /* Get first unused osal_socket structure.
     */
    mysocket_ix = osal_get_unused_socket();
    if (mysocket_ix == OSAL_ALL_USED)
    {
        osal_debug_error("osal_socket: Too many sockets");
        goto getout;
    }

    /* Clear osalSocket structure, save interface pointer.
     */
    mysocket = osal_socket + mysocket_ix;
    os_memclear(mysocket, sizeof(osalSocket));
    mysocket->hdr.iface = &osal_socket_iface;

    /* Save IP address and TCP/UDP port number.
     */
    osal_ip_from_str(mysocket->ip_address, host);
    mysocket->port_nr = (os_ushort)port_nr;

    /* Set socket use by flags
     */
    if (flags & OSAL_STREAM_UDP_MULTICAST)
    {
        mysocket->use = OSAL_SOCKET_UDP;
    }
    else if (flags & OSAL_STREAM_LISTEN)
    {
        mysocket->use = OSAL_SOCKET_SERVER;
    }
    else
    {
        mysocket->use = OSAL_SOCKET_CLIENT;
    }

    /* Do the actual work with WizChip.
     */
    osal_make_sockets();

    /* Check is socket got closed by osal_make_sockets (failed connect).
     */
    if (mysocket->use == OSAL_SOCKET_UNUSED)
    {
        goto getout;
    }

    /* Success. Set status code and return socket structure pointer
       casted to stream pointer.
	 */
	if (status) *status = OSAL_SUCCESS;
    return (osalStream)mysocket;

getout:
    /* Set status code and return NULL pointer to indicate failure.
	 */
    if (status) *status = rval;
	return OS_NULL;
}


/**
****************************************************************************************************

  @brief Close socket.
  @anchor osal_socket_close

  The osal_socket_close() function closes a socket, which was creted by osal_socket_open() 
  function. All resource related to the socket are freed. Any attemp to use the socket after
  this call may result crash.

  @param   stream Stream pointer representing the socket. After this call stream pointer will
		   point to invalid memory location.
  @return  None.

****************************************************************************************************
*/
void osal_socket_close(
	osalStream stream)
{
    osalSocket *mysocket;

    if (stream == OS_NULL) return;
    mysocket = (osalSocket*)stream;
    if (mysocket->use == OSAL_SOCKET_UNUSED)
    {
        return;
    }

    if (mysocket->state != OSAL_SOCKET_NOT_CONFIGURED)
    {
        disconnect(mysocket->port_ix);
        close(mysocket->port_ix);
        mysocket->state = OSAL_SOCKET_NOT_CONFIGURED;
    }

    mysocket->use = OSAL_SOCKET_UNUSED;
}


/**
****************************************************************************************************

  @brief Accept connection from listening socket.
  @anchor osal_socket_open

  The osal_socket_accept() function accepts an incoming connection from listening socket.

  @param   stream Stream pointer representing the listening socket.
  @param   status Pointer to integer into which to store the function status code. Value
		   OSAL_SUCCESS (0) indicates that new connection was successfully accepted.
		   The value OSAL_STATUS_NO_NEW_CONNECTION indicates that no new incoming 
		   connection, was accepted.  All other nonzero values indicate an error,
           See @ref osalStatus "OSAL function return codes" for full list.
		   This parameter can be OS_NULL, if no status code is needed. 
  @param   flags Flags for creating the socket. Define OSAL_STREAM_DEFAULT for normal operation.
		   See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Stream pointer representing the socket, or OS_NULL if the function failed.

****************************************************************************************************
*/
osalStream osal_socket_accept(
	osalStream stream,
	osalStatus *status,
	os_int flags)
{
    osalSocket *mysocket, *newsocket;
    os_short index, s;
    osalStatus rval = OSAL_STATUS_FAILED;

    if (stream == OS_NULL) goto getout;
    mysocket = (osalSocket*)stream;
    if (mysocket->use != OSAL_SOCKET_SERVER)
    {
        osal_debug_error("osal_socket: Socket is not listening");
        goto getout;
    }
    if (mysocket->state != OSAL_SOCKET_CONNECTED)
    {
        rval = OSAL_STATUS_NO_NEW_CONNECTION;
        goto getout;
    }

    s = getSn_SR(mysocket->port_ix);
    switch (s)
    {
        case SOCK_LISTEN:
        case SOCK_SYNRECV:
            osal_debug_error("getSnSR?");
            goto getout;

        case SOCK_CLOSE_WAIT:
        case SOCK_CLOSED:
        default:
            // close(mysocket->port_ix);
            mysocket->state = OSAL_SOCKET_NOT_CONFIGURED;
            osal_make_sockets();
            rval = OSAL_STATUS_NO_NEW_CONNECTION;
            goto getout;

        case SOCK_ESTABLISHED:
            break;
    }

    /* Get first unused osal_socket structure.
     */
    index = osal_get_unused_socket();
    if (index == OSAL_ALL_USED)
    {
        osal_debug_error("osal_socket: Too many sockets, cannot accept more");
        goto getout;
    }
    newsocket = osal_socket + index;

    newsocket->use = OSAL_SOCKET_CLIENT;
    newsocket->state = OSAL_SOCKET_CONNECTED;
    newsocket->port_ix = mysocket->port_ix;
    newsocket->port_nr = mysocket->port_nr;
    newsocket->local_port_nr = mysocket->local_port_nr;

    mysocket->state = OSAL_SOCKET_NOT_CONFIGURED;

    osal_make_sockets();

    if (status) *status = OSAL_SUCCESS;
    return (osalStream)newsocket;

getout:
	/* Set status code and return NULL pointer.
	 */
    if (status) *status = rval;
	return OS_NULL;
}


/**
****************************************************************************************************

  @brief Flush the socket.
  @anchor osal_socket_flush

  The osal_socket_flush() function flushes data to be written to stream.

  IMPORTANT, FLUSH MUST BE CALLED: The osal_stream_flush(<stream>, OSAL_STREAM_DEFAULT) must
  be called when select call returns even after writing or even if nothing was written, or
  periodically in in single thread mode. This is necessary even if no data was written
  previously, the socket may have stored buffered data to avoid blocking.

  @param   stream Stream pointer representing the socket.
  @param   flags See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_socket_flush(
	osalStream stream,
	os_int flags)
{
	return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Write data to socket.
  @anchor osal_socket_write

  The osal_socket_write() function writes up to n bytes of data from buffer to socket.

  @param   stream Stream pointer representing the socket.
  @param   buf Pointer to the beginning of data to place into the socket.
  @param   n Maximum number of bytes to write. 
  @param   n_written Pointer to integer into which the function stores the number of bytes 
		   actually written to socket,  which may be less than n if there is not enough space
		   left in the socket. If the function fails n_written is set to zero.
  @param   flags Flags for the function.
		   See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_socket_write(
    osalStream stream,
	const os_uchar *buf,
	os_memsz n,
	os_memsz *n_written,
	os_int flags)
{
    osalSocket *mysocket;
    int bytes, port_ix, s;

    *n_written = 0;

    if (stream == OS_NULL) return OSAL_STATUS_FAILED;
    mysocket = (osalSocket*)stream;
    if (mysocket->use != OSAL_SOCKET_CLIENT)
    {
        osal_debug_error("osal_socket: Socket can not be read");
        return OSAL_STATUS_FAILED;
    }

    port_ix = mysocket->port_ix;

    s = getSn_SR(port_ix);
    if (s != SOCK_ESTABLISHED)
    {
        return (s == SOCK_BUSY || s == SOCK_SYNSENT || s == SOCK_INIT) ? OSAL_SUCCESS : OSAL_STATUS_FAILED;
    }

    bytes = getSn_TX_FSR(port_ix);
    if (bytes < 0) return OSAL_STATUS_FAILED;
    if (n < bytes) bytes = n;

    // if (bytes > 0)
    {
        n = send(port_ix, (void*)buf, bytes);
        *n_written = n;

#if OSAL_TRACE >= 3
        if (n > 0) osal_trace("Data written to socket");
#endif
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Read data from socket.
  @anchor osal_socket_read

  The osal_socket_read() function reads up to n bytes of data from socket into buffer. 

  @param   stream Stream pointer representing the socket.
  @param   buf Pointer to buffer to read into.
  @param   n Maximum number of bytes to read. The data buffer must large enough to hold
		   at least this many bytes. 
  @param   n_read Pointer to integer into which the function stores the number of bytes read, 
           which may be less than n if there are fewer bytes available. If the function fails 
		   n_read is set to zero.
  @param   flags Flags for the function, use OSAL_STREAM_DEFAULT (0) for default operation. 
		   The OSAL_STREAM_PEEK flag causes the function to return data in socket, but nothing
		   will be removed from the socket.
		   See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_socket_read(
	osalStream stream,
	os_uchar *buf,
	os_memsz n,
	os_memsz *n_read,
	os_int flags)
{
	osalSocket *mysocket;
    int bytes, s, port_ix;

    *n_read = 0;

    if (stream == OS_NULL) return OSAL_STATUS_FAILED;
    mysocket = (osalSocket*)stream;
    if (mysocket->use != OSAL_SOCKET_CLIENT)
    {
        osal_debug_error("osal_socket: Socket can not be read");
        return OSAL_STATUS_FAILED;
    }
    port_ix = mysocket->port_ix;

    s = getSn_SR(port_ix);
    if (s != SOCK_ESTABLISHED)
    {
        return (/* s == SOCK_BUSY || */ s == SOCK_SYNSENT || s == SOCK_INIT) ? OSAL_SUCCESS : OSAL_STATUS_FAILED;
    }

    bytes = getSn_RX_RSR(port_ix);

    if (bytes < 0)
    {
        bytes = 0;
    }

    if (bytes)
    {
        if (bytes < n)
        {
            n = bytes;
        }

        bytes = recv(port_ix, buf, n);
        *n_read = bytes;
    }


#if OSAL_TRACE >= 3
    if (bytes > 0) osal_trace("Data received from socket");
#endif

    *n_read = bytes;
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Get socket parameter.
  @anchor osal_socket_get_parameter

  The osal_socket_get_parameter() function gets a parameter value.

  @param   stream Stream pointer representing the socket.
  @param   parameter_ix Index of parameter to get.
		   See @ref osalStreamParameterIx "stream parameter enumeration" for the list.
  @return  Parameter value.

****************************************************************************************************
*/
os_long osal_socket_get_parameter(
	osalStream stream,
	osalStreamParameterIx parameter_ix)
{
	/* Call the default implementation
	 */
	return osal_stream_default_get_parameter(stream, parameter_ix);
}


/**
****************************************************************************************************

  @brief Set socket parameter.
  @anchor osal_socket_set_parameter

  The osal_socket_set_parameter() function gets a parameter value.

  @param   stream Stream pointer representing the socket.
  @param   parameter_ix Index of parameter to get.
		   See @ref osalStreamParameterIx "stream parameter enumeration" for the list.
  @param   value Parameter value to set.
  @return  None.

****************************************************************************************************
*/
void osal_socket_set_parameter(
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

  @brief Get first unused osal_socket.
  @anchor osal_get_unused_socket

  The osal_get_unused_socket() function finds index of first unused osal_socket item in
  osal_socket array.

  @return Index of the first unused item in osal_socket array, OSAL_ALL_USED if all are in use.

****************************************************************************************************
*/
static os_short osal_get_unused_socket(void)
{
    os_short index;

    for (index = 0; index < OSAL_MAX_SOCKETS; index++)
    {
        if (osal_socket[index].use == OSAL_SOCKET_UNUSED) return index;
    }
    return OSAL_ALL_USED;
}


/**
****************************************************************************************************

  @brief Get WIZ chip port index which is not in use.
  @anchor osal_get_unused_w5500_port

  The osal_get_unused_w5500_port() function finds index of first unused socket port on w5500 chip.

  @return Port index 0 ... 7, or OSAL_ALL_USED if all are in use.

****************************************************************************************************
*/
static os_short osal_get_unused_w5500_port(
    void)
{
    osalSocket *mysocket;
    os_boolean used[OSAL_NRO_W5500_PORTS];
    int index, port_ix;

    os_memclear(used, sizeof(used));
    for (index = 0; index < OSAL_MAX_SOCKETS; index++)
    {
        mysocket = osal_socket + index;

        if (mysocket->use == OSAL_SOCKET_UNUSED ||
            mysocket->state == OSAL_SOCKET_NOT_CONFIGURED)
        {
            continue;
        }

        port_ix = mysocket->port_ix;
        if (port_ix >= 0 && port_ix < OSAL_NRO_W5500_PORTS)
        {
            used[port_ix] = OS_TRUE;
        }
    }

    for (port_ix = 0; port_ix < OSAL_NRO_W5500_PORTS; port_ix++)
    {
        if (!used[port_ix]) return port_ix;
    }
    return OSAL_ALL_USED;
}


/**
****************************************************************************************************

  @brief Convert string to binary MAC or IP address.
  @anchor osal_str_to_bin

  The osal_mac_from_str() converts string representation of MAC or IP address to binary.

  @param   x Pointer to byte array into which to store the address.
  @param   n Size of x in bytes. 4 or 6 bytes.
  @param   str Input, MAC or IP address as string.
  @param   c Separator character.
  @param   b 10 for decimal numbers (IP address) or 16 for hexadecimal numbers (MAC).
  @return  OS_TRUE if successfull.

****************************************************************************************************
*/
static int osal_str_to_bin(
    uint8_t *x,
    os_short n,
    const os_char* str,
    os_char c,
    os_short b)
{
    os_int i;

    for (i = 0; i < n; i++)
    {
        if (b == 10)
        {
            x[i] = (uint8_t)osal_string_to_int(str, NULL);
        }
        else
        {
            x[i] = (uint8_t)osal_hex_string_to_int(str, NULL);
        }
        str = os_strchr((os_char*)str, c);
        if (str == NULL) break;
        ++str;
    }
    return i + 1 == n;
}


/**
****************************************************************************************************

  @brief Convert string to binary IP address.
  @anchor osal_ip_from_str

  The osal_ip_from_str() converts string representation of IP address to binary.
  If the function fails, binary IP address is left unchanged.

  @param   ip Pointer where to store IP as binary.
  @param   str Input, IP address as string.
  @return  None.

****************************************************************************************************
*/
static void osal_ip_from_str(
    uint8_t *ip,
    const os_char *str)
{
    uint8_t buf[4];
    os_short i;

    if (osal_str_to_bin(buf, sizeof(buf), str, '.', 10))
    {
        for (i = 0; i<(os_short)sizeof(buf); i++) ip[i] = buf[i];
    }
#if OSAL_DEBUG
    else
    {
        osal_debug_error("IP string error");
    }
#endif
}


/**
****************************************************************************************************

  @brief Convert string to binary MAC address.
  @anchor osal_mac_from_str

  The osal_mac_from_str() converts string representation of MAC address to binary.
  If the function fails, binary MAC is left unchanged.

  @param   mac Pointer to byte array into which to store the MAC.
  @param   str Input, MAC address as string.
  @return  None.

****************************************************************************************************
*/
static void osal_mac_from_str(
    uint8_t mac[6],
    const char* str)
{
    uint8_t buf[6];

    if (osal_str_to_bin(buf, sizeof(buf), str, '-', 16))
    {
        os_memcpy(mac, buf, sizeof(buf));
    }
#if OSAL_DEBUG
    else
    {
        osal_debug_error("MAC string error");
    }
#endif
}


/**
****************************************************************************************************

  @brief Initialize sockets.
  @anchor osal_socket_initialize

  The osal_socket_initialize() initializes the underlying sockets library. This used either DHCP,
  or statical configuration parameters.

  @return  None.

****************************************************************************************************
*/
void osal_socket_initialize(
	void)
{
    /* Clear memory. Necessary, many micro-controller systems do not clear memory at soft reboot.
     */
    os_memclear(osal_socket, sizeof(osal_socket));

    /* Set flag indicating that the socket library has been initialized and clear other flags.
     */
    osal_sockets_initialized = OS_TRUE;
    osal_w5500_chip_initialized = OS_FALSE;
    osal_network_configured = OS_FALSE;
}


/**
****************************************************************************************************

  @brief Shut down sockets.
  @anchor osal_socket_shutdown

  The osal_socket_shutdown() shuts down the underlying sockets library.

  @return  None.

****************************************************************************************************
*/
void osal_socket_shutdown(
    void)
{
}


/**
****************************************************************************************************

  @brief Keep the sockets library alive.
  @anchor osal_socket_maintain

  The osal_socket_maintain() function should be called periodically to maintain sockets
  library.

  @return  None.

****************************************************************************************************
*/
#if OSAL_SOCKET_MAINTAIN_NEEDED
void osal_socket_maintain(
    void)
{
//    Ethernet.maintain();
}
#endif

static void osal_w5500_select_chip(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
}


static void osal_w5500_deselect_chip(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
}

static uint8_t osal_w5500_spi_receive_byte(void)
{
    uint8_t byte;
    HAL_SPI_Receive(&hspi1, &byte, 1, 2500);
    return byte;
}

static void osal_w5500_spi_send_byte(uint8_t byte)
{
    HAL_SPI_Transmit(&hspi1, &byte, 1, 2500);
}


/**
****************************************************************************************************

  @brief Check if wiz chip has been initialized and wire is plugger to Wiz network connector.
  @anchor osal_w5500_verify_physical_link

  The osal_w5500_verify_physical_link() function checks if we have our Wiz chip is communicating
  and physically connected to network.

  @return  OS_TRUE if connected, OS_FALSE if not.

****************************************************************************************************
*/
static os_boolean osal_w5500_verify_physical_link(void)
{
    uint8_t linkstate;
    ctlwizchip(CW_GET_PHYLINK, &linkstate);
    return (os_boolean)(linkstate != PHY_LINK_OFF);
}


/**
****************************************************************************************************

  @brief Initialize Wiznet w5500 chip.
  @anchor osal_initialize_wiz_chip

  The osal_initialize_wiz_chip() sets up SPI communication to Wizchip, initializes the chip and
  sets the MAC address.

  @return  None.

****************************************************************************************************
*/
static void osal_initialize_wiz_chip(
    void)
{
    os_timer start_t;
    uint8_t mac[6];

    /* The buffers array of sets socket buffer size for each of eight socket ports
     * of the w5500 chip. Same 2kB is used for every socket for both sending and receiving.
     */
    static uint8_t buffers[] = {2, 2, 2, 2, 2, 2, 2, 2};

    /* Set pointers tp chip select and receive/send functions to Wiznet library.
       Setup socket transmit and receive buffer sizes.
     */
    reg_wizchip_cs_cbfunc(osal_w5500_select_chip, osal_w5500_deselect_chip);
    reg_wizchip_spi_cbfunc(osal_w5500_spi_receive_byte, osal_w5500_spi_send_byte);
    if (wizchip_init(buffers, buffers) == -1)
    {
        osal_debug_error("wizchip_init() failed");
        return;
    }

    /* Wait 12 seconds for w5500 chip to boot.
     */
    os_get_timer(&start_t);
    do
    {
        if (osal_w5500_verify_physical_link()) break;
    }
    while (!os_elapsed(&start_t, 12000));

    /* Convert MAC for string to binary. Convert default MAC first (will always succeed),
     * in case MAC set is errornous.
     */
    osal_mac_from_str(mac, OSAL_MAC_DEFAULT);
    osal_mac_from_str(mac, osal_net_iface.mac);

    /* Set MAC address.
     */
    setSHAR(mac);

    osal_w5500_chip_initialized = OS_TRUE;
}


/**
****************************************************************************************************

  @brief Set IP address and other network parameters.
  @anchor osal_setup_network

  The osal_setup_network() writes the network address and other network parameters to the
  w5500 chip.

  @return  None.

****************************************************************************************************
*/
static void osal_setup_network(
    void)
{
    static wiz_NetInfo ni;

    os_memclear(&ni, sizeof(ni));
    osal_ip_from_str(ni.ip, OSAL_IP_ADDRESS_DEFAULT);
    osal_ip_from_str(ni.ip, osal_net_iface.ip_address);

    osal_ip_from_str(ni.gw, OSAL_GATEWAY_ADDRESS_DEFAULT);
    osal_ip_from_str(ni.gw, osal_net_iface.gateway_address);

    osal_ip_from_str(ni.sn, OSAL_SUBNET_MASK_DEFAULT);
    osal_ip_from_str(ni.sn, osal_net_iface.subnet_mask);
    ni.dhcp = osal_net_iface.dhcp ? NETINFO_DHCP : NETINFO_STATIC;

    if (osal_net_iface.dhcp)
    {
        osal_start_dhcp();
        osal_make_sockets();
    }

    else
    {
        wizchip_setnetinfo(&ni);
    }

    osal_network_configured = OS_TRUE;
}


/**
****************************************************************************************************

  @brief Start DHCP.
  @anchor osal_setup_network

  The osal_setup_network() writes the network address and other network parameters to the
  w5500 chip.

  @return  None.

****************************************************************************************************
*/
static void osal_start_dhcp(
    void)
{

}


/**
****************************************************************************************************

  @brief Start UDP on specified socket.
  @anchor osal_start_udp_socket

  The osal_start_udp_socket() reserved unused WIZ socket port number and stars UDP prodocol on it.

  @param   mysocket Pointer to socket structure.
  @return  None.

****************************************************************************************************
*/
static void osal_start_udp_socket(
    osalSocket *mysocket)
{
    os_short port_ix;

    port_ix = osal_get_unused_w5500_port();
    if (port_ix == OSAL_ALL_USED)
    {
        osal_debug_error("Unable to start UDP, all WIZ ports taken");
        return;
    }

    if (socket(port_ix, Sn_MR_UDP, mysocket->port_nr, 0) != port_ix)
    {
        osal_debug_error("Unable to set up UDP socket");
        return;
    }
    if (listen(port_ix) != SOCK_OK)
    {
        osal_debug_error("Listen UDP failed");
        close(port_ix);
        return;
    }

    mysocket->port_ix = port_ix;
    mysocket->local_port_nr = mysocket->port_nr;
    mysocket->state = OSAL_SOCKET_CONNECTED;
}


/**
****************************************************************************************************

  @brief Start listening socket.
  @anchor osal_start_udp_socket

  The osal_listen_server_socket() reserved unused WIZ socket port number and stars listening
  for socket connections.

  @param   mysocket Pointer to socket structure.
  @return  None.

****************************************************************************************************
*/
static void osal_listen_server_socket(
    osalSocket *mysocket)
{
    os_short port_ix;

    port_ix = osal_get_unused_w5500_port();
    if (port_ix == OSAL_ALL_USED)
    {
        osal_debug_error("Unable to listen, all WIZ ports taken");
        return;
    }

    if (socket(port_ix, Sn_MR_TCP, mysocket->port_nr, SF_IO_NONBLOCK) != port_ix)
    {
        osal_debug_error("Unable to set up listening socket");
        return;
    }
    if (listen(port_ix) != SOCK_OK)
    {
        osal_debug_error("Listen TCP failed");
        close(port_ix);
        return;
    }

    mysocket->port_ix = port_ix;
    mysocket->local_port_nr = mysocket->port_nr;
    mysocket->state = OSAL_SOCKET_CONNECTED;
}


/**
****************************************************************************************************

  @brief Find unused local socket port number.
  @anchor osal_find_free_outgoing_port

  The osal_find_free_outgoing_port() function finds an unused socket port, starting from port
  number 1500. I am uncertain is this really necessary.

  @return  Unused socket port number.

****************************************************************************************************
*/
static os_ushort osal_find_free_outgoing_port(
    void)
{
    osalSocket *mysocket;
    const os_ushort base_port_nr = 1500;
    os_boolean used[OSAL_NRO_W5500_PORTS];
    int index;
    os_ushort port_offset;

    os_memclear(used, sizeof(used));
    for (index = 0; index < OSAL_MAX_SOCKETS; index++)
    {
        mysocket = osal_socket + index;

        if (mysocket->use == OSAL_SOCKET_UNUSED ||
            mysocket->state == OSAL_SOCKET_NOT_CONFIGURED)
        {
            continue;
        }

        port_offset = mysocket->local_port_nr - base_port_nr;
        if (port_offset < OSAL_NRO_W5500_PORTS)
        {
            used[port_offset] = OS_TRUE;
        }
    }

    for (port_offset = 0; port_offset < OSAL_NRO_W5500_PORTS; port_offset++)
    {
        if (!used[port_offset]) return base_port_nr + port_offset;
    }

    return base_port_nr;
}


/**
****************************************************************************************************

  @brief Start connecting client socket.
  @anchor osal_connect_client_socket

  The osal_connect_client_socket() reserved unused WIZ socket port number and stars connecting it.

  @param   mysocket Pointer to socket structure.
  @return  None.

****************************************************************************************************
*/
static void osal_connect_client_socket(
    osalSocket *mysocket)
{
    os_short port_ix;
    os_ushort local_port_nr;
    int rval;

    port_ix = osal_get_unused_w5500_port();
    if (port_ix == OSAL_ALL_USED)
    {
        osal_debug_error("Unable to connect, all WIZ ports taken");
        goto failed;
    }

    local_port_nr = osal_find_free_outgoing_port();

    if (socket(port_ix, Sn_MR_TCP, local_port_nr, SF_IO_NONBLOCK) != port_ix)
    {
        osal_debug_error("Unable to set up connecting socket");
        goto failed;
    }

    rval = connect(port_ix, mysocket->ip_address, mysocket->port_nr);
    if (rval != SOCK_OK && rval != SOCK_BUSY)
    {
        osal_debug_error("TCP connect failed");
        // close(port_ix);
        goto failed;
    }

    mysocket->port_ix = port_ix;
    mysocket->local_port_nr = local_port_nr;
    mysocket->state = OSAL_SOCKET_CONNECTED;
    return;

failed:
    mysocket->use = OSAL_SOCKET_UNUSED;
}


/**
****************************************************************************************************

  @brief Open the sockets.
  @anchor osal_make_sockets

  The osal_make_sockets() function actually opens sockets.

  @return  None.

****************************************************************************************************
*/
static void osal_make_sockets(
    void)
{
    osalSocket *mysocket;
    int index;

    for (index = 0; index < OSAL_MAX_SOCKETS; index++)
    {
        mysocket = osal_socket + index;

        /* Skip socket ports which are already configured.
         */
        if (mysocket->state != OSAL_SOCKET_NOT_CONFIGURED)
        {
            continue;
        }

        switch (mysocket->use)
        {
            default:
            case OSAL_SOCKET_UNUSED:
                goto nextsocket;

            case OSAL_SOCKET_CLIENT:
                osal_connect_client_socket(mysocket);
                break;

            case OSAL_SOCKET_SERVER:
                osal_listen_server_socket(mysocket);
                break;

            case OSAL_SOCKET_UDP:
                osal_start_udp_socket(mysocket);
                break;

            case OSAL_SOCKET_DHCP:
                break;
        }
nextsocket:;
    }
}

/**
****************************************************************************************************

  @brief Open the sockets.
  @anchor osal_make_sockets

  The osal_make_sockets() function actually opens sockets.

  @return  None.

****************************************************************************************************
*/
#if 0
static void osal_reboot_network(
    void)
{
    osalSocket *mysocket;
    int index;

    for (index = 0; index < OSAL_MAX_SOCKETS; index++)
    {
        mysocket = osal_socket + index;

        switch (mysocket->use)
        {
            default:
            case OSAL_SOCKET_UNUSED:
                goto nextsocket;

            case OSAL_SOCKET_CLIENT:
                mysocket->use = OSAL_SOCKET_UNUSED;
                mysocket->state = OSAL_SOCKET_NOT_CONFIGURED;
                break;

            case OSAL_SOCKET_SERVER:
            case OSAL_SOCKET_UDP:
            case OSAL_SOCKET_DHCP:
                mysocket->state = OSAL_SOCKET_NOT_CONFIGURED;
                break;
        }
    }

    reboot wiz chip

    reconfigure network
}
#endif


#if OSAL_FUNCTION_POINTER_SUPPORT

/** Stream interface for OSAL sockets. This is structure osalStreamInterface filled with
    function pointers to OSAL sockets implementation.
 */
osalStreamInterface osal_socket_iface
 = {osal_socket_open,
	osal_socket_close,
	osal_socket_accept,
	osal_socket_flush,
	osal_stream_default_seek,
	osal_socket_write,
	osal_socket_read,
	osal_stream_default_write_value,
	osal_stream_default_read_value,
	osal_socket_get_parameter,
	osal_socket_set_parameter,
    OS_NULL};

#endif

#endif
