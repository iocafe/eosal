/**

  @file    socket/arduino/osal_socket_arduino_wifi.cpp
  @brief   OSAL stream API layer to use Arduino WiFi sockets.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    15.11.2019

  WiFi connectivity. Implementation of OSAL stream API and general network functionality
  using Arduino's wifi socket API. This work in both single and multi threaded systems.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
/* Force tracing on for this source file.
 */
/*  #undef OSAL_TRACE
#define OSAL_TRACE 3 */

#include "eosalx.h"
#if OSAL_SOCKET_SUPPORT==OSAL_SOCKET_ARDUINO_WIFI

#include <Arduino.h>
#include <WiFi.h>

/* Global network setup. Micro-controllers typically have one (or two)
   network interfaces. The network interface configuration is managed
   here, not by operating system.
 */
osalNetworkInterface osal_net_iface
  = {"METAL",              /* host_name */
     "192.168.1.201",      /* ip_address */
     "255.255.255.0",      /* subnet_mask */
     "192.168.1.254",      /* gateway_address */
     "8.8.8.8",            /* dns_address */
     "66-7F-18-67-A1-D3",  /* mac */
     0};                   /* dhcp */

/* Socket library initialized flag.
 */
os_boolean osal_sockets_initialized = OS_FALSE;

/* WiFi connected flag.
 */
os_boolean osal_wifi_connected = OS_FALSE;

/* Client sockets.
 */
#define OSAL_MAX_CLIENT_SOCKETS 6
static WiFiClient osal_client[OSAL_MAX_CLIENT_SOCKETS];
static os_boolean osal_client_used[OSAL_MAX_CLIENT_SOCKETS];

/* Listening server sockets.
 */
#define OSAL_MAX_SERVER_SOCKETS 2
static WiFiServer osal_server[OSAL_MAX_SERVER_SOCKETS]
    = {WiFiServer(IOC_DEFAULT_SOCKET_PORT),
       WiFiServer(IOC_DEFAULT_SOCKET_PORT+1)};
static os_boolean osal_server_used[OSAL_MAX_SERVER_SOCKETS];


/* UDP sockets.
 */
/* #define OSAL_MAX_UDP_SOCKETS 2
static WiFiUDP osal_udp[OSAL_MAX_UDP_SOCKETS];
static os_boolean osal_udp_used[OSAL_MAX_UDP_SOCKETS];
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

/* Array of socket structures for every possible WizNet sockindex
 */
static osalSocket osal_socket[OSAL_MAX_SOCKETS];


/* Prototypes for forward referred static functions.
 */
static void osal_socket_setup_ring_buffer(
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
    os_short mysocket_ix, ix;
    os_boolean is_ipv6;
    osalSocket *mysocket;
    osalStatus rval = OSAL_STATUS_FAILED;

    /* If not initialized.
     */
    if (!osal_sockets_initialized)
    {
        if (status) *status = OSAL_STATUS_FAILED;
        return OS_NULL;
    }

	/* Get host name or numeric IP address and TCP port number from parameters.
	 */
    osal_socket_get_host_name_and_port(parameters,
        &port_nr, host, sizeof(host), &is_ipv6, flags, IOC_DEFAULT_SOCKET_PORT);

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

    /* *** If UDP socket ***
     */
    if (flags & OSAL_STREAM_UDP_MULTICAST)
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

        /* The "osal_server[ix] = EthernetServer(port_nr)" doesn't work because it
         * allocates temporary EtherNetServer object, and then in addition to port
         * number copies some rubbish uninitialized from uninitialized  member
         * variables to statically allocated EtherNetServer object.
         * To solve this, function setport added to class. If setport() function
         * if not found during compilation, then public member function
         * "inline void setport(uint16_t port) {_port = port;}"
         * needs to be added to EthernetServer class.
         * Note: Same for WiFi
         */
        /* osal_server[ix].setport((uint16_t)port_nr); XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
        osal_server[ix].begin();

        osal_server_used[ix] = OS_TRUE;
        mysocket->use = OSAL_SOCKET_SERVER;
        mysocket->index = ix;
        mysocket->sockindex = OSAL_ALL_USED;
        osal_trace2("Listening socket opened");
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

        if (osal_client[ix].connect(host, port_nr) == 0)
        {
            osal_debug_error("osal_socket: Socket connect failed");
            rval = OSAL_STATUS_CONNECTION_REFUSED;
            goto getout;
        }

        osal_client[ix].setNoDelay(true);
        osal_client[ix].setTimeout(0);
        osal_socket_setup_ring_buffer(mysocket);

        osal_client_used[ix] = OS_TRUE;
        mysocket->use = OSAL_SOCKET_CLIENT;
        mysocket->index = ix;
        mysocket->sockindex = osal_client[ix].fd();

        osal_trace2("Connecting socket");
        osal_trace2(host);
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


static void osal_socket_setup_ring_buffer(
    osalSocket *mysocket)
{
    os_memsz sz;

   /* 1760 selected for TCP sockets, foced over TCP packet size limit */
    mysocket->buf = os_malloc(1760, &sz);
    mysocket->buf_sz = sz;
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
            osal_client[ix].stop();
            break;

        default:
            osal_debug_error("osal_socket: Socket can not be closed?");
            break;
    }

    /* Free ring buffer, if any, and mark socket unused (clear whole struct to be safe).
     */
    os_free(mysocket->buf, mysocket->buf_sz);
    os_memclear(mysocket, sizeof(osalSocket));
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
    os_char *remote_ip_addr,
    os_memsz remote_ip_addr_sz,
    osalStatus *status,
	os_int flags)
{
    osalSocket *mysocket;
    os_short mysocket_ix, six, cix, j;
    MY_SOCKIX_TYPE sockindex;
    osalStatus rval = OSAL_STATUS_FAILED;

    if (stream == OS_NULL) goto getout;
    mysocket = (osalSocket*)stream;
    if (mysocket->use != OSAL_SOCKET_SERVER)
    {
        osal_debug_error("osal_socket: Socket is not listening");
        goto getout;
    }

    /* Get first unused osal_socket structure.
     */
    mysocket_ix = osal_get_unused_socket();
    if (mysocket_ix == OSAL_ALL_USED)
    {
        osal_debug_error("osal_socket: Too many sockets, cannot accept more");
        goto getout;
    }
    mysocket = osal_socket + mysocket_ix;
    six = mysocket->index;

    /* Get first unused osal_client index.
     */
    cix = osal_get_unused_client();
    if (cix == OSAL_ALL_USED)
    {
        osal_debug_error("osal_socket: Too many clients, cannot accept more");
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
        sockindex = osal_client[cix].fd();

        for (j = 0; j<OSAL_MAX_SOCKETS; j++)
        {
           if (osal_socket[j].use == OSAL_SOCKET_UNUSED) continue;

           if (sockindex == osal_socket[j].sockindex)
           {
                /* osal_trace2("Socket port with data rejected because it is already in use"); */
                rval = OSAL_STATUS_NO_NEW_CONNECTION;
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
        osal_client_used[cix] = OS_TRUE;
        osal_trace2("Incoming socket accepted");

        if (remote_ip_addr) *remote_ip_addr = '\0';

        osal_client[cix].setNoDelay(true);
        osal_client[cix].setTimeout(0);
        osal_socket_setup_ring_buffer(mysocket);

        /* Return socket pointer.
         */
        return (osalStream)mysocket;
    }
    rval = OSAL_STATUS_NO_NEW_CONNECTION;

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
    osalSocket *mysocket;
    os_short head, tail, wrnow;
    os_memsz nwr;
    osalStatus status;

    if (stream == OS_NULL) return OSAL_STATUS_FAILED;

    mysocket = (osalSocket*)stream;
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
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

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
        if (errno == EAGAIN)
        {
            osal_trace2("osal_socket_write: Again");
            return OSAL_SUCCESS;
        }
        osal_debug_error("osal_socket_write: Disconnected");
        return OSAL_STATUS_STREAM_CLOSED;
    }
    *n_written = bytes;

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
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

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

    if (stream)
    {
        /* Cast stream pointer to socket structure pointer.
         */
        mysocket = (osalSocket*)stream;
        osal_debug_assert(mysocket->hdr.iface == &osal_socket_iface);

        /* Check for errorneous arguments.
         */
        if (n < 0 || buf == OS_NULL)
        {
            status = OSAL_STATUS_FAILED;
            goto getout;
        }

        /* Special case. Writing 0 bytes will trigger write callback by worker thread.
         */
        if (n == 0)
        {
            status = OSAL_SUCCESS;
            goto getout;
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
                    if (status) goto getout;
                    if (nwr == wrnow) tail = 0;
                    else tail += (os_short)nwr;
                }

                if (head > tail)
                {
                    wrnow = head - tail;

                    status = osal_socket_write2(mysocket, rbuf+tail, wrnow, &nwr, flags);
                    if (status) goto getout;
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
    status = OSAL_STATUS_FAILED;

getout:
    *n_written = 0;
    return status;
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
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

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
    os_int read_now, bytes;
    os_short ix;

    *n_read = 0;

    if (stream == OS_NULL) return OSAL_STATUS_FAILED;
    mysocket = (osalSocket*)stream;
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
        if (errno == EAGAIN)
        {
            osal_trace2("osal_socket_read: Again");
            return OSAL_SUCCESS;
        }
        osal_debug_error("osal_socket_read: Disconnected");
        return OSAL_STATUS_STREAM_CLOSED;
    }

#if OSAL_TRACE >= 3
    if (bytes > 0) osal_trace("Data received from socket");
#endif

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
        if (!osal_client_used[index]) return index;
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
        if (!osal_server_used[index]) return index;
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


/**
****************************************************************************************************

  @brief Convert string to binary MAC address.
  @anchor osal_arduino_mac_from_str

  The osal_arduino_mac_from_str() converts string representation of MAC address to binary.
  If the function fails, binary MAC is left unchanged.

  @param   mac Pointer to byte array into which to store the MAC.
  @param   str Input, MAC address as string.
  @return  None.

****************************************************************************************************
*/
static void osal_arduino_mac_from_str(
    byte mac[6],
    const char* str)
{
    os_uchar buf[6];
    os_short i;

    if (osal_mac_from_str(buf, str) == OSAL_SUCCESS)
    {
        for (i = 0; i < sizeof(buf); i++) mac[i] = buf[i];
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

  @param   nic Pointer to array of network interface structures. Can be OS_NULL to use default.
  @param   n_nics Number of network interfaces in nic array.
  @return  None.

****************************************************************************************************
*/
void osal_socket_initialize(
    osalNetworkInterface *nic,
    os_int n_nics)
{
    const char* ssid     = "julian";
    const char* password = "talvi333";


    /* Initialize only once.
     */
    IPAddress
        ip_address(192, 168, 1, 201),
        dns_address(8, 8, 8, 8),
        gateway_address(192, 168, 1, 254),
        subnet_mask(255, 255, 255, 0);

    byte
        mac[6] = {0x66, 0x7F, 0x18, 0x67, 0xA1, 0xD3};

    osal_sockets_initialized = OS_TRUE;
    os_memclear(osal_socket, sizeof(osal_socket));
    os_memclear(osal_client_used, sizeof(osal_client_used));
    os_memclear(osal_server_used, sizeof(osal_server_used));

    osal_arduino_mac_from_str(mac, osal_net_iface.mac);

    /* Initialize using static configuration.
     */
    osal_arduino_ip_from_str(ip_address, osal_net_iface.ip_address);
    osal_arduino_ip_from_str(dns_address, osal_net_iface.dns_address);
    osal_arduino_ip_from_str(gateway_address, osal_net_iface.gateway_address);
    osal_arduino_ip_from_str(subnet_mask, osal_net_iface.subnet_mask);

    //DO NOT TOUCH
     //  This is here to force the ESP32 to reset the WiFi and initialise correctly.
     Serial.print("WIFI status = ");
     Serial.println(WiFi.getMode());
     WiFi.disconnect(true);
     delay(1000);
     WiFi.mode(WIFI_STA);
     delay(1000);
     Serial.print("WIFI status = ");
     Serial.println(WiFi.getMode());
     // End silly stuff !!!

     WiFi.mode(WIFI_STA);
     WiFi.disconnect();
     osal_wifi_connected = (os_boolean)(WiFi.status() == WL_CONNECTED);
     delay(100);

    /* Start the WiFi.
     */
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    osal_trace("Wifi initialized ");

// Here WE should convert IP address to string.
#if OSAL_TRACE
        IPAddress ip = WiFi.localIP();
        String strip = DisplayAddress(ip);
        osal_trace(strip.c_str());
#endif

    /* Set socket library initialized flag.
     */
    osal_sockets_initialized = OS_TRUE;
}


os_boolean osal_is_wifi_initialized(
    void)
{
    return osal_sockets_initialized;
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
    if (osal_sockets_initialized)
    {
        WiFi.disconnect();
        osal_sockets_initialized = OS_FALSE;
    }
}


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


#if OSAL_FUNCTION_POINTER_SUPPORT

/** Stream interface for OSAL sockets. This is structure osalStreamInterface filled with
    function pointers to OSAL sockets implementation.
 */
const osalStreamInterface osal_socket_iface
 = {osal_socket_open,
	osal_socket_close,
	osal_socket_accept,
	osal_socket_flush,
	osal_stream_default_seek,
	osal_socket_write,
	osal_socket_read,
	osal_stream_default_write_value,
	osal_stream_default_read_value,
    osal_stream_default_get_parameter,
    osal_stream_default_set_parameter,
    OS_NULL};

#endif

#endif
