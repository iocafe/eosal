/**

  @file    socket/duino/osal_sam_socket_wifi.cpp
  @brief   OSAL stream API layer to use Arduino SAM WiFi sockets.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  WiFi connectivity. Implementation of OSAL stream API and general network functionality
  using Arduino's wifi socket API. This work in both single and multi threaded systems,
  but all sockets need to be handled by one thread.

  Features:
  - WiFiMulti to allows automatic switching between two known wifi networks. Notice that if
    two wifi networks are specified in NIC connfiguration, then static network configuration
    cannot be used and DHCP will be enabled.

  Notes:
  - Wifi.config() function in ESP does not follow same argument order as arduino. This
    can create problem if using static IP address.
  - Static WiFi IP address doesn't work for ESP32. This seems to be a bug in espressif Arduino
    support (replacing success check with 15 sec delay will patch it). Wait for espressif
    updates, ESP32 is still quite new.

  MISSING - TO BE DONE
  - DNS to resolve host names
  - UDP multicasts for "ligthouse"
  - Nagle needs to follow NODELAY flags, now always disabled

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#ifdef OSAL_ARDUINO
#if (OSAL_SOCKET_SUPPORT & OSAL_SOCKET_MASK) == OSAL_SAM_WIFI_API

#include <WiFi101.h>


/* Two known wifi networks to select from in NIC configuration.
 */
static os_boolean osal_wifi_multi_on = OS_FALSE;


typedef struct
{
    os_char ip_address[OSAL_HOST_BUF_SZ];

    IPAddress
        dns_address,
        dns_address_2,
        gateway_address,
        subnet_mask;

    os_boolean no_dhcp;

    os_char wifi_net_name[OSAL_WIFI_PRM_SZ];
    os_char wifi_net_password[OSAL_WIFI_PRM_SZ];

}
osalArduinoNetParams;

/* The osal_socket_initialize() stores application's network setting here. The values in
   are then used and changed by initialization to reflect initialized state.
 */
static osalArduinoNetParams osal_wifi_nic;

/* Socket library initialized flag.
 */
os_boolean osal_sockets_initialized = OS_FALSE;

/* WiFi connected flag.
 */
os_boolean osal_wifi_connected = OS_FALSE;

enum {
    OSAL_WIFI_INIT_STEP1,
    OSAL_WIFI_INIT_STEP2,
    OSAL_WIFI_INIT_STEP3
}
osal_wifi_init_step;

static os_boolean osal_wifi_init_failed_once;
static os_boolean osal_wifi_init_failed_now;
static os_boolean osal_wifi_was_connected;
static os_timer osal_wifi_step_timer;
static os_timer osal_wifi_boot_timer;

/** Possible socket uses.
 */
typedef enum
{
    OSAL_UNUSED_STATE = 0,
    OSAL_PREPARED_STATE,
    OSAL_RUNNING_STATE,
    OSAL_FAILED_STATE
}
osalSocketState;

/* Client sockets.
 */
#define OSAL_MAX_CLIENT_SOCKETS 6
static WiFiClient osal_client[OSAL_MAX_CLIENT_SOCKETS];
static osalSocketState osal_client_state[OSAL_MAX_CLIENT_SOCKETS];

/* Listening server sockets.
 */
#define OSAL_MAX_SERVER_SOCKETS 2
static WiFiServer osal_server[OSAL_MAX_SERVER_SOCKETS]
    = {WiFiServer(IOC_DEFAULT_SOCKET_PORT),
       WiFiServer(IOC_DEFAULT_SOCKET_PORT+1)};
static osalSocketState osal_server_state[OSAL_MAX_SERVER_SOCKETS];


/* UDP sockets.
 */
/* #define OSAL_MAX_UDP_SOCKETS 2
static WiFiUDP osal_udp[OSAL_MAX_UDP_SOCKETS];
static osalSocketState osal_udp_used[OSAL_MAX_UDP_SOCKETS];
*/

/** Index to mark that there are no unused items in array.
 */
#define OSAL_ALL_USED 127

/** Maximum number of WizNet sockets.
 */
#define OSAL_MAX_SOCKETS 8

/** Possible socket uses.
 */
typedef enum
{
    OSAL_SOCKET_UNUSED = 0,
    OSAL_SOCKET_CLIENT,
    OSAL_SOCKET_SERVER,
    OSAL_SOCKET_UDP
}
osalSocketUse;

#define MY_SOCKIX_TYPE int


/** Arduino specific socket structure to store information.
 */
typedef struct osalSocket
{
    /** A stream structure must start with this generic stream header structure, which contains
        parameters common to every stream.
     */
    osalStreamHeader hdr;

    /** Host name or IP addrss.
     */
    os_char host[OSAL_IPADDR_SZ];
    os_int port_nr;

    /** TRUE for IP v6 address, FALSE for IO v4.
     */
    os_boolean is_ipv6;

    /** Nonzero if socket [sockindex] is used. One of OSAL_SOCKET_UNUSED, OSAL_SOCKET_CLIENT,
        OSAL_SOCKET_SERVER or OSAL_SOCKET_UDP.
     */
    osalSocketUse use;

    /** Index in osal_client, osal_server or osal_udp array, depending on "use" member.
     */
    os_short index;

    /** Wiznet chip's or other socket port index.
     */
    MY_SOCKIX_TYPE sockindex;

    /** Ring buffer, OS_NULL if not used.
     */
    os_char *buf;

    /** Buffer size in bytes.
     */
    os_short buf_sz;

    /** Head index. Position in buffer to which next byte is to be written. Range 0 ... buf_sz-1.
     */
    os_short head;

    /** Tail index. Position in buffer from which next byte is to be read. Range 0 ... buf_sz-1.
     */
    os_short tail;
}
osalSocket;

/* Array of socket structures for every possible  sockindex
 */
static osalSocket osal_socket[OSAL_MAX_SOCKETS];


/* Prototypes for forward referred static functions.
 */
static void osal_socket_setup_ring_buffer(
    osalSocket *mysocket);

static osalStatus osal_socket_really_connect(
    osalSocket *mysocket);

static osalStatus osal_socket_really_listen(
    osalSocket *mysocket);

static osalStatus osal_socket_write2(
    osalSocket *mysocket,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags);

static os_short osal_get_unused_socket(void);
static os_short osal_get_unused_client(void);
static os_short osal_get_unused_server(void);
// static os_short osal_get_unused_udp(void);

void osal_socket_on_wifi_connect(void);
void osal_socket_on_wifi_disconnect(void);

static void osal_socket_start_wifi_init(void);

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
          IPv6 address is automatically recognized from numeric address like
          "2001:0db8:85a3:0000:0000:8a2e:0370:7334", but not when address is specified as string
          nor for empty IP specifying only port to listen. Use brackets around IP address
          to mark IPv6 address, for example "[localhost]:12345", or "[]:12345" for empty IP.

  @param  option Not used for sockets, set OS_NULL.

  @param  status Pointer to integer into which to store the function status code. Value
          OSAL_SUCCESS (0) indicates success and all nonzero values indicate an error.

          This parameter can be OS_NULL, if no status code is needed.

  @param  flags Flags for creating the socket. Bit fields, combination of:
          - OSAL_STREAM_CONNECT: Connect to specified socket port at specified IP address.
          - OSAL_STREAM_LISTEN: Open a socket to listen for incoming connections.
          - OSAL_STREAM_MULTICAST: Open a UDP multicast socket.
          - OSAL_STREAM_NO_SELECT: Open socket without select functionality.
          - OSAL_STREAM_SELECT: Open socket with select functionality.
          - OSAL_STREAM_TCP_NODELAY: Disable Nagle's algorithm on TCP socket.
          - OSAL_STREAM_NO_REUSEADDR: Disable reusability of the socket descriptor.

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
    os_short mysocket_ix, ix, i;
    osalSocket *mysocket;
    os_char addr[16], nbuf[OSAL_NBUF_SZ];
    osalStatus rval = OSAL_STATUS_FAILED, wifi_status;

    /* If not initialized or wifi is pending.
     */
    wifi_status = osal_are_sockets_initialized();
    if (wifi_status == OSAL_STATUS_FAILED)
    {
        rval = wifi_status;
        goto getout;
    }

    /* Get first unused osal_socket structure.
     */
    mysocket_ix = osal_get_unused_socket();
    if (mysocket_ix == OSAL_ALL_USED)
    {
        osal_debug_error("osal_socket: Too many sockets");
        goto getout;
    }

    /* Clear osalSocket structure and save interface pointer.
     */
    mysocket = osal_socket + mysocket_ix;
    os_memclear(mysocket, sizeof(osalSocket));
    mysocket->hdr.iface = &osal_socket_iface;

    /* Get host name or numeric IP address and TCP port number from parameters.
       The host buffer must be released by calling os_free() function,
       unless if host is OS_NULL (unpecified).
     */
    osal_socket_get_ip_and_port(parameters, addr, sizeof(addr),
        &mysocket->port_nr, &mysocket->is_ipv6,
        flags, IOC_DEFAULT_SOCKET_PORT);

    for (i = 0; i<4; i++)
    {
        if (i) os_strncat(mysocket->host, ".", OSAL_IPADDR_SZ);
        osal_int_to_str(nbuf, sizeof(nbuf), addr[i]);
        os_strncat(mysocket->host, nbuf, OSAL_IPADDR_SZ);
    }

    /* *** If UDP socket ***
     */
    if (flags & OSAL_STREAM_MULTICAST)
    {
    }

    /* *** listening for socket port ***
     */
    else if (flags & OSAL_STREAM_LISTEN)
    {
        ix = osal_get_unused_server();
        if (ix == OSAL_ALL_USED)
        {
            osal_debug_error("osal_socket: Too many server sockets");
            goto getout;
        }

        mysocket->use = OSAL_SOCKET_SERVER;
        mysocket->index = ix;
        mysocket->sockindex = OSAL_ALL_USED;
        osal_server_state[ix] = OSAL_PREPARED_STATE;

        if (wifi_status == OSAL_SUCCESS)
        {
            if (osal_socket_really_listen(mysocket))
            {
                os_memclear(mysocket, sizeof(osalSocket));
                osal_server_state[ix] = OSAL_UNUSED_STATE;
                goto getout;
            }
        }
    }

    /* *** connecting for socket port ***
     */
    else
    {
        ix = osal_get_unused_client();
        if (ix == OSAL_ALL_USED)
        {
            osal_debug_error("osal_socket: Too many client sockets");
            goto getout;
        }

        osal_client_state[ix] = OSAL_PREPARED_STATE;
        mysocket->use = OSAL_SOCKET_CLIENT;
        mysocket->index = ix;

        if (wifi_status == OSAL_SUCCESS)
        {
            if (osal_socket_really_connect(mysocket))
            {
                os_memclear(mysocket, sizeof(osalSocket));
                osal_client_state[ix] = OSAL_UNUSED_STATE;
                goto getout;
            }
        }
        osal_resource_monitor_increment(OSAL_RMON_SOCKET_CONNECT_COUNT);
    }

    /* Success. Set status code and return socket structure pointer
       casted to stream pointer.
     */
    if (status) *status = OSAL_SUCCESS;
    osal_resource_monitor_increment(OSAL_RMON_SOCKET_COUNT);
    return (osalStream)mysocket;

getout:
    /* Set status code and return NULL pointer to indicate failure.
     */
    if (status) *status = rval;
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Setup rind buffer for transmitted data.
  @anchor osal_socket_setup_ring_buffer

  Ring buffer is used to avoid sending small TCP packages when multiple packages. We wish to
  disable Nagle's alhorithm, thus we need to worry about this.

****************************************************************************************************
*/
static void osal_socket_setup_ring_buffer(
    osalSocket *mysocket)
{
    os_memsz sz;

    if (mysocket->buf == OS_NULL)
    {
        /* 1760 selected for TCP sockets, foced over TCP packet size limit.
         */
        mysocket->buf = os_malloc(1760, &sz);
        mysocket->buf_sz = sz;
    }
}


/**
****************************************************************************************************

  @brief Actually connect the socket.
  @anchor osal_socket_really_connect

  A socket can be connected later than application requests, for example if wifi is not yet
  up when application opens the socket, and the socket is finally open on wifi network connect.
  Listening server sockets need to be closed when switching from WiFi network to another
  this function Other

****************************************************************************************************
*/
static osalStatus osal_socket_really_connect(
    osalSocket *mysocket)
{
    os_short ix;
    ix = mysocket->index;

    osal_debug_assert(osal_client_state[ix] == OSAL_PREPARED_STATE);
    osal_trace2("Connecting socket");
    osal_trace2(mysocket->host);

    if (osal_client[ix].connect(mysocket->host, mysocket->port_nr) == 0)
    {
        osal_debug_error("osal_socket: Socket connect failed");
        return OSAL_STATUS_CONNECTION_REFUSED;
    }

    /* N/A
    osal_client[ix].setNoDelay(true);
    */

    osal_client[ix].setTimeout(0);
    osal_socket_setup_ring_buffer(mysocket);

    /* N/A     SOCKET fd(){return _socket;} WiFiClient.h */
    mysocket->sockindex = osal_client[ix].fd();

    osal_client_state[ix] = OSAL_RUNNING_STATE;
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Actually start listening for a socket port.
  @anchor osal_socket_really_listen

  A listening socket can be opened later than application requests, for example if wifi is not yet
  up when application opens the listening socket. And the socket is finally open on wifi network
  connect. Other case is when switching from wifi network to another or walking too far from
  wifi base station, the connections need to be closed and reopened.

****************************************************************************************************
*/
static osalStatus osal_socket_really_listen(
    osalSocket *mysocket)
{
    os_short ix;
    ix = mysocket->index;

    osal_debug_assert(osal_server_state[ix] == OSAL_PREPARED_STATE);

    /* N/A    void setport(uint16_t nr){_port = nr;} */
    osal_server[ix].setport((uint16_t)mysocket->port_nr);
    osal_server[ix].begin();
    osal_trace_int("Listening TCP port ", mysocket->port_nr);


    osal_server_state[ix] = OSAL_RUNNING_STATE;
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Called to check socket status before sending, receiving or flushing.
  @anchor osal_socket_check

  The osal_socket_check() function...

  @return  OSAL_SUCCESS if we are connected to a wifi network, proceed with operation.
           OSAL_PENDING If currently connecting and have not never failed to connect so far.
                Return OSAL_SUCCESS with no bytes transferred.
           OSAL_STATUS_FALED No wifi connection of socket has been closed because of
                break in wifi connection.

****************************************************************************************************
*/
osalStatus osal_socket_check(
    osalSocket *mysocket)
{
    osalStatus  s;
    os_int  ix;

    s = osal_are_sockets_initialized();
    if (s != OSAL_SUCCESS) return s;

    ix = mysocket->index;
    if (mysocket->use == OSAL_SOCKET_CLIENT)
    {
        if (osal_client_state[ix] == OSAL_RUNNING_STATE)
            return OSAL_SUCCESS;
    }

    if (mysocket->use == OSAL_SOCKET_SERVER)
    {
        if (osal_server_state[ix] == OSAL_RUNNING_STATE)
            return OSAL_SUCCESS;
    }

    return OSAL_STATUS_FAILED;
    return 0;
}


/**
****************************************************************************************************

  @brief Close socket.
  @anchor osal_socket_close

  The osal_socket_close() function closes a socket, which was created by osal_socket_open()
  function. All resource related to the socket are freed. Any attempt to use the socket after
  this call may result crash.

  @param   stream Stream pointer representing the socket. After this call stream pointer will
           point to invalid memory location.
  @return  None.

****************************************************************************************************
*/
void osal_socket_close(
    osalStream stream,
    os_int flags)
{
    osalSocket *mysocket;
    os_short ix;

    if (stream == OS_NULL) return;
    mysocket = (osalSocket*)stream;
    if (mysocket->use == OSAL_SOCKET_UNUSED)
    {
        osal_debug_error("osal_socket: Socket closed twice");
        return;
    }

    osal_trace2("closing socket");
    ix = mysocket->index;
    switch (mysocket->use)
    {
        case OSAL_SOCKET_CLIENT:
            switch (osal_client_state[ix])
            {
                case OSAL_UNUSED_STATE:
                case OSAL_PREPARED_STATE:
                    break;

                case OSAL_RUNNING_STATE:
                case OSAL_FAILED_STATE:
                    osal_client[ix].stop();
                    mysocket->sockindex = 0;
                    break;
            }

            osal_client_state[ix] = OSAL_UNUSED_STATE;
            break;

        case OSAL_SOCKET_SERVER:
            switch (osal_server_state[ix])
            {
                case OSAL_UNUSED_STATE:
                case OSAL_PREPARED_STATE:
                case OSAL_FAILED_STATE:
                    break;

                case OSAL_RUNNING_STATE:

                    /* N/A
                    osal_server[ix].stop();
                    */

                    mysocket->sockindex = 0;
                    break;
            }
            osal_server_state[ix] = OSAL_UNUSED_STATE;
            break;

        default:
            osal_debug_error("osal_socket: Socket can not be closed?");
            break;
    }

    /* Free ring buffer, if any, and mark socket unused (clear whole struct to be safe).
     */
    os_free(mysocket->buf, mysocket->buf_sz);
    os_memclear(mysocket, sizeof(osalSocket));
    osal_resource_monitor_decrement(OSAL_RMON_SOCKET_COUNT);
}


/**
****************************************************************************************************

  @brief Accept connection from listening socket.
  @anchor osal_socket_open

  The osal_socket_accept() function accepts an incoming connection from listening socket.

  @param   stream Stream pointer representing the listening socket.
  @param   status Pointer to integer into which to store the function status code. Value
           OSAL_SUCCESS (0) indicates that new connection was successfully accepted.
           The value OSAL_NO_NEW_CONNECTION indicates that no new incoming
           connection, was accepted.  All other nonzero values indicate an error,

           This parameter can be OS_NULL, if no status code is needed.
  @param   flags Flags for creating the socket. Define OSAL_STREAM_DEFAULT for normal operation.
           See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Stream pointer representing the socket, or OS_NULL if the function failed.

****************************************************************************************************
*/
osalStream osal_socket_accept(
    osalStream stream,
    os_char *remote_ip_addr,
    os_memsz remote_ip_addr_sz,
    osalStatus *status,
    os_int flags)
{
    osalSocket *mysocket;
    os_short mysocket_ix, six, cix, j;
    MY_SOCKIX_TYPE sockindex;
    osalStatus rval = OSAL_STATUS_FAILED, wifi_rval;

    mysocket = (osalSocket*)stream;
    wifi_rval = osal_socket_check(mysocket);
    if (wifi_rval)
    {
        rval = (wifi_rval == OSAL_PENDING
            ? OSAL_NO_NEW_CONNECTION : wifi_rval);
        goto getout;
    }

    if (mysocket->use != OSAL_SOCKET_SERVER)
    {
        osal_debug_error("osal_socket: Not a listening socket");
        goto getout;
    }
    six = mysocket->index;

    /* Get first unused osal_socket structure.
     */
    mysocket_ix = osal_get_unused_socket();
    if (mysocket_ix == OSAL_ALL_USED)
    {
        osal_debug_error("osal_socket: Too many sockets, cannot accept more");
        goto getout;
    }
    mysocket = osal_socket + mysocket_ix;

    /* Get first unused osal_client index.
     */
    cix = osal_get_unused_client();
    if (cix == OSAL_ALL_USED)
    {
        osal_debug_error("osal_socket: Too many clients, can't accept more");
        goto getout;
    }

    /* Try to sort of "accept" a new client.
     */
    osal_client[cix] = osal_server[six].available();
    if (osal_client[cix])
    {
        /* The Arduino's available() is not same as accept(). It returns a socket
           with data to read. This may be a socket which is already in use,
           this we must skip the used ones using sockindex.
         */

        /* N/A     SOCKET fd(){return _socket;} WiFiClient.h */
        sockindex = osal_client[cix].fd();

        for (j = 0; j<OSAL_MAX_SOCKETS; j++)
        {
           if (j == cix) continue;

           if (sockindex == osal_socket[j].sockindex)
           {
                /* osal_trace2("Socket port with data rejected because it is already in use"); */
                rval = OSAL_NO_NEW_CONNECTION;
                goto getout;
           }
        }

        /* Set up osalSocket structure and save interface pointer.
         */
        os_memclear(mysocket, sizeof(osalSocket));
        mysocket->hdr.iface = &osal_socket_iface;
        mysocket->use = OSAL_SOCKET_CLIENT;
        mysocket->index = cix;
        mysocket->sockindex = sockindex;
        osal_client_state[cix] = OSAL_RUNNING_STATE;
        osal_trace2("Incoming socket accepted");

        if (remote_ip_addr) *remote_ip_addr = '\0';

        /* N/A
        osal_client[cix].setNoDelay(true);
        */

        osal_client[cix].setTimeout(0);
        osal_socket_setup_ring_buffer(mysocket);

        /* Success, return socket pointer.
         */
        if (status) *status = OSAL_SUCCESS;
        osal_resource_monitor_increment(OSAL_RMON_SOCKET_COUNT);
        osal_resource_monitor_increment(OSAL_RMON_SOCKET_CONNECT_COUNT);
        return (osalStream)mysocket;
    }
    rval = OSAL_NO_NEW_CONNECTION;

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
           indicate an error.

****************************************************************************************************
*/
osalStatus osal_socket_flush(
    osalStream stream,
    os_int flags)
{
    osalSocket *mysocket;
    os_short head, tail, wrnow;
    os_memsz nwr;
    osalStatus status;

    mysocket = (osalSocket*)stream;
    status = osal_socket_check(mysocket);
    if (status) return status == OSAL_PENDING ? OSAL_SUCCESS: status;

    head = mysocket->head;
    tail = mysocket->tail;
    if (head != tail)
    {
        if (head < tail)
        {
            wrnow = mysocket->buf_sz - tail;
            status = osal_socket_write2(mysocket, mysocket->buf + tail, wrnow, &nwr, flags);
            if (status) return status;
            if (nwr == wrnow) tail = 0;
            else tail += (os_short)nwr;
        }

        if (head > tail)
        {
            wrnow = head - tail;
            status = osal_socket_write2(mysocket, mysocket->buf + tail, wrnow, &nwr, flags);
            if (status) return status;
            tail += (os_short)nwr;
        }

        if (tail == head)
        {
            tail = head = 0;
        }

        mysocket->head = head;
        mysocket->tail = tail;
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Write data to socket.
  @anchor osal_socket_write

  The osal_socket_write2() function writes up to n bytes of data from buffer to socket.

  @param   stream Stream pointer representing the socket.
  @param   buf Pointer to the beginning of data to place into the socket.
  @param   n Maximum number of bytes to write.
  @param   n_written Pointer to integer into which the function stores the number of bytes
           actually written to socket,  which may be less than n if there is not enough space
           left in the socket. If the function fails n_written is set to zero.
  @param   flags Flags for the function.
           See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error.

****************************************************************************************************
*/
static osalStatus osal_socket_write2(
    osalSocket *mysocket,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags)
{
    os_int bytes;
    os_short ix;

    *n_written = 0;

    ix = mysocket->index;

    if (!osal_client[ix].connected())
    {
        osal_debug_error("osal_socket_write: Not connected");
        return OSAL_STATUS_FAILED;
    }
    if (n == 0) return OSAL_SUCCESS;

    bytes = osal_client[ix].write(buf, n);
    if (bytes < 0)
    {

        /* N/A
        if (errno == EAGAIN)
        {
            osal_trace2("osal_socket_write: Again");
            return OSAL_SUCCESS;
        }
        */

        osal_debug_error("osal_socket_write: Disconnected");
        return OSAL_STATUS_STREAM_CLOSED;
    }
    *n_written = bytes;
    osal_resource_monitor_update(OSAL_RMON_TX_TCP, bytes);

#if OSAL_TRACE >= 3
    if (bytes > 0) osal_trace("Data written to socket");
#endif

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
           indicate an error.

****************************************************************************************************
*/
osalStatus osal_socket_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags)
{
    os_int count, wrnow;
    osalSocket *mysocket;
    osalStatus status;
    os_char *rbuf;
    os_short head, tail, buf_sz, nexthead;
    os_memsz nwr;
    os_boolean all_not_flushed;

    *n_written = 0;

    mysocket = (osalSocket*)stream;
    status = osal_socket_check(mysocket);
    if (status) return status == OSAL_PENDING ? OSAL_SUCCESS: status;

    /* Cast stream pointer to socket structure pointer.
     */
    mysocket = (osalSocket*)stream;
    osal_debug_assert(mysocket->hdr.iface == &osal_socket_iface);

    /* Check for errorneous arguments.
     */
    if (n < 0 || buf == OS_NULL)
    {
        return OSAL_STATUS_FAILED;
    }

    /* Special case. Writing 0 bytes will trigger write callback by worker thread.
     */
    if (n == 0)
    {
        return OSAL_SUCCESS;
    }

    if (mysocket->buf)
    {
        rbuf = mysocket->buf;
        buf_sz = mysocket->buf_sz;
        head = mysocket->head;
        tail = mysocket->tail;
        all_not_flushed = OS_FALSE;
        count = 0;

        while (osal_go())
        {
            while (n > 0)
            {
                nexthead = head + 1;
                if (nexthead >= buf_sz) nexthead = 0;
                if (nexthead == tail) break;
                rbuf[head] = *(buf++);
                head = nexthead;
                n--;
                count++;
            }

            if (n == 0 || all_not_flushed)
            {
                break;
            }

            if (head < tail)
            {
                wrnow = buf_sz - tail;

                status = osal_socket_write2(mysocket, rbuf+tail, wrnow, &nwr, flags);
                if (status) return status;
                if (nwr == wrnow) tail = 0;
                else tail += (os_short)nwr;
            }

            if (head > tail)
            {
                wrnow = head - tail;

                status = osal_socket_write2(mysocket, rbuf+tail, wrnow, &nwr, flags);
                if (status) return status;
                tail += (os_short)nwr;
            }

            if (tail == head)
            {
                tail = head = 0;
            }
            else
            {
                all_not_flushed = OS_TRUE;
            }
        }

        mysocket->head = head;
        mysocket->tail = tail;
        *n_written = count;
        return OSAL_SUCCESS;
    }

    return osal_socket_write2(mysocket, buf, n, n_written, flags);
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

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error.

****************************************************************************************************
*/
osalStatus osal_socket_read(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags)
{
    osalSocket *mysocket;
    osalStatus status;
    os_int read_now, bytes;
    os_short ix;

    *n_read = 0;

    mysocket = (osalSocket*)stream;
    status = osal_socket_check(mysocket);
    if (status) return status == OSAL_PENDING ? OSAL_SUCCESS: status;

    if (mysocket->use != OSAL_SOCKET_CLIENT)
    {
        return OSAL_STATUS_FAILED;
    }
    ix = mysocket->index;

    if (!osal_client[ix].connected())
    {
        osal_debug_error("osal_socket_read: Not connected");
        return OSAL_STATUS_STREAM_CLOSED;
    }

    read_now = osal_client[ix].available();
    if (read_now <= 0) return OSAL_SUCCESS;
    if (n < read_now) read_now = n;

    bytes = osal_client[ix].read((uint8_t*)buf, read_now);
    if (bytes < 0)
    {

        /* N/A
        if (errno == EAGAIN)
        {
            osal_trace2("osal_socket_read: Again");
            return OSAL_SUCCESS;
        }
        */

        osal_debug_error("osal_socket_read: Disconnected");
        return OSAL_STATUS_STREAM_CLOSED;
    }

#if OSAL_TRACE >= 3
    if (bytes > 0) osal_trace("Data received from socket");
#endif

    osal_resource_monitor_update(OSAL_RMON_RX_TCP, bytes);
    *n_read = bytes;
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Get first unused osal_socket.
  @anchor osal_get_unused_socket

  The osal_get_unused_socket() function finds index of first unused osal_socket item in
  osal_socket array.

  @return Index of the first unused item in osal_socket array, OSAL_ALL_USED (127) if all are in use.

****************************************************************************************************
*/
static os_short osal_get_unused_socket(void)
{
    os_short index;

    for (index = 0; index < OSAL_MAX_CLIENT_SOCKETS; index++)
    {
        if (osal_socket[index].use == OSAL_SOCKET_UNUSED) return index;
    }
    return OSAL_ALL_USED;
}


/**
****************************************************************************************************

  @brief Get first unused EthernetClient.
  @anchor osal_get_unused_client

  The osal_get_unused_client() function finds index of first unused EthernetClient item in
  osal_clients array.

  @return Index of the first unused item in osal_client array, OSAL_ALL_USED (127) if all are in use.

****************************************************************************************************
*/
static os_short osal_get_unused_client(void)
{
    os_short index;

    for (index = 0; index < OSAL_MAX_CLIENT_SOCKETS; index++)
    {
        if (!osal_client_state[index]) return index;
    }
    return OSAL_ALL_USED;
}


/**
****************************************************************************************************

  @brief Get first unused EthernetServer.
  @anchor osal_get_unused_server

  The osal_get_unused_server() function finds index of first unused EthernetServer item in
  osal_servers array.

  @return Index of the first unused item in osal_server array, OSAL_ALL_USED (127) if all are in use.

****************************************************************************************************
*/
static os_short osal_get_unused_server(void)
{
    os_short index;

    for (index = 0; index < OSAL_MAX_SERVER_SOCKETS; index++)
    {
        if (!osal_server_state[index]) return index;
    }
    return OSAL_ALL_USED;
}


/**
****************************************************************************************************

  @brief Get first unused EthernetUDP.
  @anchor osal_get_unused_udp

  The osal_get_unused_udp() function finds index of first unused EthernetUDP item in
  osal_udp array.

  @return Index of the first unused item in osal_server array, OSAL_ALL_USED (127) if all are in use.

****************************************************************************************************
*/
/* static os_short osal_get_unused_udp(void)
{
    os_short index;

    for (index = 0; index < OSAL_MAX_UDP_SOCKETS; index++)
    {
        if (!osal_udp_used[index]) return index;
    }
    return OSAL_ALL_USED;
} */


/**
****************************************************************************************************

  @brief Convert string to binary IP address.
  @anchor osal_arduino_ip_from_str

  The osal_arduino_ip_from_str() converts string representation of IP address to binary.
  If the function fails, binary IP address is left unchanged.

  @param   ip Pointer to Arduino IP address to set.
  @param   str Input, IP address as string.
  @return  None.

****************************************************************************************************
*/
static void osal_arduino_ip_from_str(
    IPAddress& ip,
    const os_char *str)
{
    os_uchar buf[4];
    os_short i;

    if (osal_ip_from_str(buf, sizeof(buf), str) == OSAL_SUCCESS)
    {
        for (i = 0; i < sizeof(buf); i++) ip[i] = buf[i];
    }
}


static String DisplayAddress(IPAddress address)
{
 return String(address[0]) + "." +
        String(address[1]) + "." +
        String(address[2]) + "." +
        String(address[3]);
}


/**
****************************************************************************************************

  @brief Initialize sockets LWIP/WizNet.
  @anchor osal_socket_initialize

  The osal_socket_initialize() initializes the underlying sockets library. This used either DHCP,
  or statical configuration parameters.

  Network interface configuration must be given to osal_socket_initialize() as when using wifi,
  because wifi SSID (wifi net name) and password are required to connect.

  @param   nic Pointer to array of network interface structures. A network interface is needed,
           and The arduino Wifi implementation
           supports only one network interface.
  @param   n_nics Number of network interfaces in nic array. 1 or more, only first NIC used.
  @param   wifi Pointer to array of WiFi network structures. This contains wifi network name (SSID)
           and password (pre shared key) pairs. Can be OS_NULL if there is no WiFi.
  @param   n_wifi Number of wifi networks network interfaces in wifi array.
  @return  None.

****************************************************************************************************
*/
#if 0
void osal_socket_initialize(
    osalNetworkInterface *nic,
    os_int n_nics,
    osalWifiNetwork *wifi,
    os_int n_wifi)
{
    if (nic == OS_NULL && n_nics < 1)
    {
        osal_debug_error("osal_socket_initialize(): No NIC configuration");
    }

    os_memclear(osal_socket, sizeof(osal_socket));
    os_memclear(osal_client_state, sizeof(osal_client_state));
    os_memclear(osal_server_state, sizeof(osal_server_state));

    os_strncpy(osal_wifi_nic.ip_address, nic[0].ip_address, OSAL_HOST_BUF_SZ);
    osal_arduino_ip_from_str(osal_wifi_nic.dns_address, nic[0].dns_address);
    osal_arduino_ip_from_str(osal_wifi_nic.dns_address_2, nic[0].dns_address_2);
    osal_arduino_ip_from_str(osal_wifi_nic.gateway_address, nic[0].gateway_address);
    osal_arduino_ip_from_str(osal_wifi_nic.subnet_mask, nic[0].subnet_mask);
    osal_wifi_nic.no_dhcp = nic[0].no_dhcp;
    os_strncpy(osal_wifi_nic.wifi_net_name, wifi[0].wifi_net_name, OSAL_WIFI_PRM_SZ);
    os_strncpy(osal_wifi_nic.wifi_net_password, wifi[0].wifi_net_password,OSAL_WIFI_PRM_SZ);

    /* Start wifi initialization.
     */
    osal_socket_start_wifi_init();

    /* Set socket library initialized flag.
     */
    osal_sockets_initialized = OS_TRUE;
    osal_global->sockets_shutdown_func = osal_socket_shutdown;
}
#endif


/**
****************************************************************************************************

  @brief Start Wifi initialisation from beginning.
  @anchor osal_socket_start_wifi_init

  The osal_socket_start_wifi_init() function starts wifi initialization. The
  initialization is continued by repeatedly called osal_are_sockets_initialized() function.

  @return  None.

****************************************************************************************************
*/
#if 0
static void osal_socket_start_wifi_init(void)
{
    /* Start the WiFi. Do not wait for the results here, we wish to allow IO to run even
       without WiFi network.
     */
    osal_trace("Commecting to Wifi network");

    /* Set socket library initialized flag, now waiting for wifi initialization. We do not lock
     * the code here to allow IO sequence, etc to proceed even without wifi.
     */
    osal_wifi_init_step = OSAL_WIFI_INIT_STEP1;
    osal_wifi_init_failed_once = OS_FALSE;

    /* Call wifi init once to move once to start it.
     */
    osal_are_sockets_initialized();
}
#endif


/**
****************************************************************************************************

  @brief Check if WiFi network is connected.
  @anchor osal_are_sockets_initialized

  Called to check if WiFi initialization has been completed and if so, the function initializes
  has been initialized and connected.

  @return  OSAL_SUCCESS if we are connected to a wifi network.
           OSAL_PENDING If currently connecting and have not never failed to connect so far.
           OSAL_STATUS_FALED No connection, at least for now.

****************************************************************************************************
*/
#if 0
osalStatus osal_are_sockets_initialized(
    void)
{
    osalStatus s;

    if (!osal_sockets_initialized) return OSAL_STATUS_FAILED;

    s = osal_wifi_init_failed_once
        ? OSAL_STATUS_FAILED : OSAL_PENDING;

    switch (osal_wifi_init_step)
    {
        case OSAL_WIFI_INIT_STEP1:
            /* The following four lines are silly stuff to reset
               the ESP32 wifi after soft reboot. I assume that this will be fixed and
               become unnecessary at some point.
             */
            osal_wifi_connected = osal_wifi_was_connected = OS_FALSE;
            osal_wifi_init_failed_now = OS_FALSE;
            os_get_timer(&osal_wifi_step_timer);
            osal_wifi_boot_timer = osal_wifi_step_timer;
            osal_wifi_init_step = OSAL_WIFI_INIT_STEP2;

            break;

        case OSAL_WIFI_INIT_STEP2:
            if (os_has_elapsed(&osal_wifi_step_timer, 100))
            {
                /* Start the WiFi.
                 */
                if (!osal_wifi_multi_on)
                {
                    /* Initialize using static configuration.
                     */
                    if (osal_wifi_nic.no_dhcp)
                    {
                        /* Some default network parameters.
                         */
                        IPAddress
                            ip_address(192, 168, 1, 195);

                        osal_arduino_ip_from_str(ip_address, osal_wifi_nic.ip_address);

                        WiFi.config(ip_address, osal_wifi_nic.dns_address,
                            osal_wifi_nic.gateway_address, osal_wifi_nic.subnet_mask);
                    }

                    WiFi.begin(osal_wifi_nic.wifi_net_name, osal_wifi_nic.wifi_net_password);
                }

                os_get_timer(&osal_wifi_step_timer);
                osal_wifi_init_step = OSAL_WIFI_INIT_STEP3;
                osal_trace("Connecting wifi");
            }
            break;

        case OSAL_WIFI_INIT_STEP3:
            osal_wifi_connected = (os_boolean) (WiFi.status() == WL_CONNECTED);

            /* If no change in connection status:
               - If we are connected or connection has never failed (boot), or
                 not connected, return appropriate status code. If not con
             */
            if (osal_wifi_connected == osal_wifi_was_connected)
            {
                if (osal_wifi_connected)
                {
                    s = OSAL_SUCCESS;
                    break;
                }

                if (osal_wifi_init_failed_now)
                {
                    s = OSAL_STATUS_FAILED;
                }

                else
                {
                    if (os_has_elapsed(&osal_wifi_step_timer, 8000))
                    {
                        osal_wifi_init_failed_now = OS_TRUE;
                        osal_wifi_init_failed_once = OS_TRUE;
                        osal_trace("Unable to connect Wifi");
                    }

                    s = osal_wifi_init_failed_once
                        ? OSAL_STATUS_FAILED : OSAL_PENDING;
                }

                break;
            }

            /* Save to detect connection state changes.
             */
            osal_wifi_was_connected = osal_wifi_connected;

            /* If this is connect
             */
            if (osal_wifi_connected)
            {
                s = OSAL_SUCCESS;
                osal_trace("Wifi network connected");
                osal_socket_on_wifi_connect();

#if OSAL_TRACE
                IPAddress ip = WiFi.localIP();
                String strip = DisplayAddress(ip);
                osal_trace(strip.c_str());
#endif
            }

            /* Otherwise this is disconnect.
             */
            else
            {
                osal_wifi_init_step = OSAL_WIFI_INIT_STEP1;
                osal_trace("Wifi network disconnected");
                osal_socket_on_wifi_disconnect();
                s = OSAL_STATUS_FAILED;
            }

            break;
    }

    return s;
}
#endif


/**
****************************************************************************************************

  @brief Called when WiFi network is connected.
  @anchor osal_socket_on_wifi_connect

  The osal_socket_on_wifi_connect() function...
  @return  None.

****************************************************************************************************
*/
#if 0
void osal_socket_on_wifi_connect(
    void)
{
    osalSocket *mysocket;
    os_int i, ix;

    for (i = 0; i<OSAL_MAX_SOCKETS; i++)
    {
        mysocket = osal_socket + i;
        ix = mysocket->index;
        switch (mysocket->use)
        {
            case OSAL_SOCKET_UNUSED:
                break;

            case OSAL_SOCKET_CLIENT:
                if (osal_client_state[ix] != OSAL_PREPARED_STATE) break;
                if (osal_socket_really_connect(mysocket))
                {
                    osal_client_state[ix] = OSAL_FAILED_STATE;
                }
                break;

            case OSAL_SOCKET_SERVER:
                if (osal_server_state[ix] != OSAL_PREPARED_STATE &&
                    osal_server_state[ix] != OSAL_FAILED_STATE)
                {
                    break;
                }
                if (osal_socket_really_listen(mysocket))
                {
                    osal_server_state[ix] = OSAL_FAILED_STATE;
                }
                break;

            case OSAL_SOCKET_UDP:
                break;
        }
    }
}
#endif

/**
****************************************************************************************************

  @brief Called when connected WiFi network is disconnected.
  @anchor osal_socket_on_wifi_disconnect

  The osal_socket_on_wifi_disconnect() function...
  @return  None.

****************************************************************************************************
*/
#if 0
void osal_socket_on_wifi_disconnect(
    void)
{
    osalSocket *mysocket;
    os_int i, ix;

    for (i = 0; i<OSAL_MAX_SOCKETS; i++)
    {
        mysocket = osal_socket + i;
        ix = mysocket->index;
        switch (mysocket->use)
        {
            case OSAL_SOCKET_UNUSED:
                break;

            case OSAL_SOCKET_CLIENT:
                if (osal_client_state[ix] != OSAL_RUNNING_STATE) break;
                osal_client_state[ix] = OSAL_FAILED_STATE;
                break;

            case OSAL_SOCKET_SERVER:
                if (osal_server_state[ix] != OSAL_RUNNING_STATE) break;

                /* N/A
                osal_server[ix].stop();
                */

                osal_server_state[ix] = OSAL_FAILED_STATE;
                mysocket->sockindex = 0;
                break;

            case OSAL_SOCKET_UDP:
                break;
        }
    }
}
#endif


/**
****************************************************************************************************

  @brief Shut down sockets.
  @anchor osal_socket_shutdown

  The osal_socket_shutdown() shuts down the underlying sockets library.

  @return  None.

****************************************************************************************************
*/
#if 0
void osal_socket_shutdown(
    void)
{
    if (osal_sockets_initialized)
    {
        WiFi.disconnect();
        osal_sockets_initialized = OS_FALSE;
    }
}
#endif


#if OSAL_SOCKET_MAINTAIN_NEEDED
/**
****************************************************************************************************

  @brief Keep the sockets library alive.
  @anchor osal_socket_maintain

  The osal_socket_maintain() function is not needed for Arduino WiFi, empty function is here
  just to allow build if OSAL_SOCKET_MAINTAIN_NEEDED is on.

  @return  None.

****************************************************************************************************
*/
void osal_socket_maintain(
    void)
{
  #warning Unnecessary OSAL_SOCKET_MAINTAIN_NEEDED=1 define, remove to save a few bytes
}
#endif


/** Stream interface for OSAL sockets. This is structure osalStreamInterface filled with
    function pointers to OSAL sockets implementation.
 */
OS_CONST osalStreamInterface osal_socket_iface
 = {OSAL_STREAM_IFLAG_NONE,
    osal_socket_open,
    osal_socket_close,
    osal_socket_accept,
    osal_socket_flush,
    osal_stream_default_seek,
    osal_socket_write,
    osal_socket_read,
    osal_stream_default_select};

#endif
#endif
