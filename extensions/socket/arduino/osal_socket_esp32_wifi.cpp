/**

  @file    socket_esp/arduino/osal_socket_esp_wifi.c
  @brief   OSAL TLS sockets API Arduino WiFi implementation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    20.1.2017

  Implementation of OSAL sockets for secure TLS sockets over WiFi within Arduino framework.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
/* Force tracing on for this source file.
 */
// #undef OSAL_TRACE
// #define OSAL_TRACE 3

#include "eosalx.h"
#if OSAL_SOCKET_SUPPORT==OSAL_SOCKET_WIFI_ESP32


#include <Arduino.h>
#include <WiFiClientSecure.h>

/* Global network setup. Micro-controllers typically have one (or two)
   network interfaces. The network interface configuration is managed
   here, not by operating system.
 */
static osalNetworkInterface osal_net_iface
  = {"BRASS",              /* host_name */
     "192.168.1.201",      /* ip_address */
     "255.255.255.0",      /* subnet_mask */
     "192.168.1.254",      /* gateway_address */
     "8.8.8.8",            /* dns_address */
     "66-7F-18-67-A1-D3",  /* mac */
     0};                   /* dhcp */


/** TLS library initialized flag.
 */
os_boolean osal_socket_esp_initialized = OS_FALSE;

/** WiFi network connected flag.
 */
static os_boolean osal_wifi_initialized;

/** WiFi network connection timer.
 */
static os_timer osal_wifi_init_timer;

/** Arduino specific socket class to store information.
 */
class osalSocket
{
    public:
    /** A stream structure must start with this generic stream header structure, which contains
        parameters common to every stream.
     */
    osalStreamHeader hdr;

    /* Arduino librarie's Wifi TLS socket client object.
     */
    WiFiClientSecure client;

    /** Nonzero if socket is used.
     */
    os_boolean used;
};

/** Maximum number of sockets.
 */
#define OSAL_MAX_SOCKETS 8

/* Array of socket structures for every possible WizNet sockindex
 */
static osalSocket osal_socket_esp[OSAL_MAX_SOCKETS];


/* Prototypes for forward referred static functions.
 */
static osalSocket *osal_get_unused_socket(void);

static os_boolean osal_is_wifi_initialized(
    void);

/**
****************************************************************************************************

  @brief Open a socket.
  @anchor osal_socket_esp_open

  The osal_socket_esp_open() function opens a socket. The socket can be either listening TCP
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
osalStream osal_socket_esp_open(
    const os_char *parameters,
    void *option,
    osalStatus *status,
    os_int flags)
{
    os_int port_nr;
    os_char host[OSAL_HOST_BUF_SZ];
    os_boolean is_ipv6;
    osalSocket *w;
    osalStatus rval = OSAL_STATUS_FAILED;

    /* Initialize sockets library, if not already initialized.
     */
    if (!osal_socket_esp_initialized)
    {
        osal_socket_esp_initialize(OS_NULL, 0, OS_NULL);
    }

    /* If WiFi network is not connected, we can do nothing.
     */
    if (!osal_is_wifi_initialized())
    {
        rval = OSAL_STATUS_PENDING;
        goto getout;
    }

    /* Get host name or numeric IP address and TCP port number from parameters.
       The host buffer must be released by calling os_free() function,
       unless if host is OS_NULL (unpecified).
     */
    osal_socket_get_host_name_and_port(parameters,
        &port_nr, host, sizeof(host), &is_ipv6, flags, IOC_DEFAULT_TLS_PORT);

    /* Get first unused osal_socket_esp structure.
     */
    w = osal_get_unused_socket();
    if (w == OS_NULL)
    {
        osal_debug_error("osal_socket_esp: Too many sockets");
        goto getout;
    }

    /* Set up client certificate, if we use one.
     */
      // w->client.setCACert(test_root_ca);  // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX WE MAY WANT TO USE THIS
      // w->client.setCertificate(bobs_key); // for client verification
      // w->client.setPrivateKey(bobs_certificate);	// for client verification

    osal_trace2_int("Connecting to TLS socket port ", port_nr);
    osal_trace2(host);

    /* Connect the socket.
     */
    if (!w->client.connect(host, port_nr))
    {
        osal_trace("Wifi: TLS socket connect failed");
        w->client.stop();
        goto getout;
    }

    os_memclear(&w->hdr, sizeof(osalStreamHeader));
    w->used = OS_TRUE;
    w->hdr.iface = &osal_socket_esp_iface;

    osal_trace2("wifi: TLS socket connected.");

    /* Success. Set status code and return socket structure pointer
       casted to stream pointer.
     */
    if (status) *status = OSAL_SUCCESS;
    return (osalStream)w;

getout:
    /* Set status code and return NULL pointer to indicate failure.
     */
    if (status) *status = rval;
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Close socket.
  @anchor osal_socket_esp_close

  The osal_socket_esp_close() function closes a socket, which was creted by osal_socket_esp_open()
  function. All resource related to the socket are freed. Any attemp to use the socket after
  this call may result crash.

  @param   stream Stream pointer representing the socket. After this call stream pointer will
           point to invalid memory location.
  @return  None.

****************************************************************************************************
*/
void osal_socket_esp_close(
    osalStream stream)
{
    osalSocket *w;

    if (stream == OS_NULL) return;
    w = (osalSocket*)stream;
    if (!w->used)
    {
        osal_debug_error("osal_socket_esp: Socket closed twice");
        return;
    }

    w->client.stop();
    w->used = OS_FALSE;
}


/**
****************************************************************************************************

  @brief Accept connection from listening socket.
  @anchor osal_socket_esp_open

  The osal_socket_esp_accept() function accepts an incoming connection from listening socket.

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
osalStream osal_socket_esp_accept(
    osalStream stream,
    os_char *remote_ip_addr,
    os_memsz remote_ip_addr_sz,
    osalStatus *status,
    os_int flags)
{
    if (status) *status = OSAL_STATUS_FAILED;
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Flush the socket.
  @anchor osal_socket_esp_flush

  The osal_socket_esp_flush() function flushes data to be written to stream.


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
osalStatus osal_socket_esp_flush(
    osalStream stream,
    os_int flags)
{
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Write data to socket.
  @anchor osal_socket_esp_write

  The osal_socket_esp_write() function writes up to n bytes of data from buffer to socket.

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
osalStatus osal_socket_esp_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags)
{
    osalSocket *w;
    int bytes;

    *n_written = 0;

    if (stream == OS_NULL) return OSAL_STATUS_FAILED;
    w = (osalSocket*)stream;
    if (!w->used)
    {
        osal_debug_error("osal_socket_esp: Unused socket");
        return OSAL_STATUS_FAILED;
    }

    if (!w->client.connected())
    {
        osal_debug_error("osal_socket_esp: Not connected");
        return OSAL_STATUS_FAILED;
    }
    if (n == 0) return OSAL_SUCCESS;

    bytes = w->client.write((uint8_t*)buf, n);
    if (bytes < 0) bytes = 0;
    *n_written = bytes;

#if OSAL_TRACE >= 3
    if (bytes > 0) osal_trace("Data written to socket");
#endif

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Read data from socket.
  @anchor osal_socket_esp_read

  The osal_socket_esp_read() function reads up to n bytes of data from socket into buffer.

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
osalStatus osal_socket_esp_read(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags)
{
    osalSocket *w;
    int bytes;

    *n_read = 0;

    if (stream == OS_NULL) return OSAL_STATUS_FAILED;
    w = (osalSocket*)stream;
    if (!w->used)
    {
        osal_debug_error("osal_socket_esp: Socket can not be read");
        return OSAL_STATUS_FAILED;
    }

    if (!w->client.connected())
    {
        osal_debug_error("osal_socket_esp: Not connected");
        return OSAL_STATUS_FAILED;
    }

    bytes = w->client.available();
    if (bytes < 0) bytes = 0;
    if (bytes)
    {
        if (bytes > n)
        {
            bytes = n;
        }

        bytes = w->client.read((uint8_t*)buf, bytes);
    }

#if OSAL_TRACE >= 3
    if (bytes > 0) osal_trace2_int("Data received from socket", bytes);
#endif

    *n_read = bytes;
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Get socket parameter.
  @anchor osal_socket_esp_get_parameter

  The osal_socket_esp_get_parameter() function gets a parameter value.

  @param   stream Stream pointer representing the socket.
  @param   parameter_ix Index of parameter to get.
           See @ref osalStreamParameterIx "stream parameter enumeration" for the list.
  @return  Parameter value.

****************************************************************************************************
*/
os_long osal_socket_esp_get_parameter(
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
  @anchor osal_socket_esp_set_parameter

  The osal_socket_esp_set_parameter() function gets a parameter value.

  @param   stream Stream pointer representing the socket.
  @param   parameter_ix Index of parameter to get.
           See @ref osalStreamParameterIx "stream parameter enumeration" for the list.
  @param   value Parameter value to set.
  @return  None.

****************************************************************************************************
*/
void osal_socket_esp_set_parameter(
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

  @brief Get first unused osal_socket_esp.
  @anchor osal_get_unused_socket

  The osal_get_unused_socket() function finds index of first unused osal_socket_esp item in
  osal_socket_esp array.

  @return Pointer to OSAL TLS socket structure, or OS_NULL if no free ones.

****************************************************************************************************
*/
static osalSocket *osal_get_unused_socket(void)
{
    int i;

    for (i = 0; i < OSAL_MAX_SOCKETS; i++)
    {
        if (!osal_socket_esp[i].used) return osal_socket_esp + i;
    }
    return OS_NULL;
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
/* static int osal_str_to_bin(
    byte *x,
    os_short n,
    const os_char* str,
    os_char c,
    os_short b)
{
    os_int i;

    for (i = 0; i < n; i++)
    {
        x[i] = (byte)strtoul(str, NULL, b);
        str = strchr(str, c);
        if (str == NULL) break;
        ++str;
    }
    return i + 1 == n;
}
*/


/**
****************************************************************************************************

  @brief Convert string to binary IP address.
  @anchor osal_ip_from_str

  The osal_ip_from_str() converts string representation of IP address to binary.
  If the function fails, binary IP address is left unchanged.

  @param   ip Pointer to Arduino IP address to set.
  @param   str Input, IP address as string.
  @return  None.

****************************************************************************************************
*/
/* static void osal_ip_from_str(
    IPAddress& ip,
    const os_char *str)
{
    byte buf[4];
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
*/


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
/* static void osal_mac_from_str(
    byte mac[6],
    const char* str)
{
    byte buf[6];

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
*/


/**
****************************************************************************************************

  @brief Initialize sockets LWIP/WizNet.
  @anchor osal_socket_esp_initialize

  The osal_socket_esp_initialize() initializes the underlying sockets library. This used either DHCP,
  or statical configuration parameters.

  @param   nic Pointer to array of network interface structures. Can be OS_NULL to use default.
           This implementation sets up only nic[0].
  @param   n_nics Number of network interfaces in nic array.
  @param   prm Parameters related to TLS. Can OS_NULL for client if not needed.

  @return  None.

****************************************************************************************************
*/
void osal_socket_esp_initialize(
    osalNetworkInterface *nic,
    os_int n_nics,
    osalTLSParam *prm)
{
    const os_char *wifi_net_name = "bean24";
    const os_char *wifi_net_password = "talvi333";
    int i;

    /* Initialize only once.
     */
    /* IPAddress
        ip_address(192, 168, 1, 201),
        dns_address(8, 8, 8, 8),
        gateway_address(192, 168, 1, 254),
        subnet_mask(255, 255, 255, 0);

    byte
        mac[6] = {0x66, 0x7F, 0x18, 0x67, 0xA1, 0xD3};
    */

    /* Clear Get parameters. Use defaults if not set.
     */
    for (i = 0; i < OSAL_MAX_SOCKETS; i++)
    {
        osal_socket_esp[i].used = OS_FALSE;
    }

    /* Get parameters. Use defaults if not set.
     */
    if (nic == OS_NULL) n_nics = 0;
    if (n_nics >= 1)
    {
        if (*nic->wifi_net_name != '\0') wifi_net_name = nic->wifi_net_name;
        if (*nic->wifi_net_password != '\0') wifi_net_password = nic->wifi_net_password;
    }

    osal_socket_esp_initialized = OS_TRUE;

    // osal_mac_from_str(mac, osal_net_iface.mac);

    /* Initialize using static configuration.
    osal_ip_from_str(ip_address, osal_net_iface.ip_address);
    osal_ip_from_str(dns_address, osal_net_iface.dns_address);
    osal_ip_from_str(gateway_address, osal_net_iface.gateway_address);
    osal_ip_from_str(subnet_mask, osal_net_iface.subnet_mask);
     */

    /* Start the WiFi. Do not wait for the results here, we wish to allow IO to run even
       without WiFi network.
     */
    osal_trace("Commecting to Wifi network");
    osal_trace(wifi_net_name);
    WiFi.begin(wifi_net_name, wifi_net_password);

    /* Set TLS library initialized flag, now waiting for wifi initialization. We do not lock
     * the code here to allow IO sequence, etc to proceed even without wifi.
     */
    osal_socket_esp_initialized = OS_TRUE;
    osal_wifi_initialized = OS_FALSE;
}


/**
****************************************************************************************************

  @brief Check if WiFi network is connected.
  @anchor osal_is_wifi_initialized

  The osal_is_wifi_initialized() function is used when opening or staring to listen for incoming
  connections to make sure that WiFi network is connected.

  @return  OS_TRUE if we are connected to WiFi network, or OS_FALSE otherwise.

****************************************************************************************************
*/
static os_boolean osal_is_wifi_initialized(
    void)
{
    if (!osal_socket_esp_initialized) return OS_FALSE;
    if (!osal_wifi_initialized)
    {
        /* If WiFi is not connected, just return failure.
         */
        if (WiFi.status() != WL_CONNECTED)
        {
            if (os_elapsed(&osal_wifi_init_timer, 500))
            {
                osal_trace2("Waiting for wifi");
                os_get_timer(&osal_wifi_init_timer);
            }
            return OS_FALSE;
        }
        osal_trace("Wifi network connected");

        /* Here WE should convert IP address to string.
           ip_address = Ethernet.localIP(); */

        /* Mark that Wifi is intialized.
         */
        osal_wifi_initialized = OS_TRUE;
    }
    return OS_TRUE;
}


/**
****************************************************************************************************

  @brief Shut down sockets.
  @anchor osal_socket_esp_shutdown

  The osal_socket_esp_shutdown() shuts down the underlying sockets library.

  @return  None.

****************************************************************************************************
*/
void osal_socket_esp_shutdown(
    void)
{
    if (osal_socket_esp_initialized)
    {
        WiFi.disconnect();
        osal_socket_esp_initialized = OS_FALSE;
    }
}


/**
****************************************************************************************************

  @brief Keep the sockets library alive.
  @anchor osal_socket_esp_maintain

  The osal_socket_esp_maintain() function should be called periodically to maintain sockets
  library.

  @return  None.

****************************************************************************************************
*/
void osal_socket_esp_maintain(
    void)
{
//    Ethernet.maintain();
}


#if OSAL_FUNCTION_POINTER_SUPPORT

/** Stream interface for OSAL sockets. This is structure osalStreamInterface filled with
    function pointers to OSAL sockets implementation.
 */
const osalStreamInterface osal_socket_esp_iface
 = {osal_socket_esp_open,
    osal_socket_esp_close,
    osal_socket_esp_accept,
    osal_socket_esp_flush,
    osal_stream_default_seek,
    osal_socket_esp_write,
    osal_socket_esp_read,
    osal_stream_default_write_value,
    osal_stream_default_read_value,
    osal_socket_esp_get_parameter,
    osal_socket_esp_set_parameter,
    OS_NULL};

#endif

#endif