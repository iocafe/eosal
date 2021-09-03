/**

  @file    socket/common/osal_lwip_netconn_socket.cpp
  @brief   OSAL stream API layer to use lwIP netconn API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Ethernet/WiFi connectivity. Implementation of OSAL stream API and general network functionality
  using lwIP librarie's netconn API. This work in both single and multi threaded systems.

  NOT READY, JUST PLANNED

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/

#include "eosalx.h"
#if (OSAL_SOCKET_SUPPORT & OSAL_SOCKET_MASK) == OSAL_LWIP_NETCONN_API

/* Force tracing on for this source file.
 */
// #undef OSAL_TRACE
// #define OSAL_TRACE 3
#ifndef OSAL_ESPIDF_FRAMEWORK
#endif

extern "C"{
#include "lwip/opt.h"
#include "lwip/inet.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/err.h"

// #include "lwip/netif.h"
// #include "lwip/udp.h"
// #include "lwip/stats.h"
// #include "lwip/prot/dhcp.h"
// #include "lwip/debug.h"
}

#include <WiFi.h>

/* Queue sizes.
 */
#define OSAL_SOCKET_RX_BUF_SZ 1450
#define OSAL_SOCKET_TX_BUF_SZ 1450

/* Global network setup. Micro-controllers typically have one (or two)
   network interfaces. The network interface configuration is managed
   here, not by operating system.
 */
static osalNetworkInterfaceOld osal_net_iface
  = {"BRASS",              /* host_name */
     "192.168.1.201",      /* ip_address */
     "255.255.255.0",      /* subnet_mask */
     "192.168.1.254",      /* gateway_address */
     "8.8.8.8",            /* dns_address */
     "66-7F-18-67-A1-D3",  /* mac */
     0};                   /* dhcp */


/** Sockets library initialized flag.
 */
os_boolean osal_sockets_initialized;

/** WiFi network connected flag.
 */
static os_boolean osal_wifi_initialized;

/** WiFi network connection timer.
 */
static os_timer osal_wifi_init_timer;

/** Arduino specific socket class to store information.
 */
typedef struct osalSocket
{
    /** A stream structure must start with this generic stream header structure, which contains
        parameters common to every stream.
     */
    osalStreamHeader hdr;

    /** Nonzero if socket structure is reserved by a thread.
     */
    volatile os_boolean reserved;

    /** Nonzero if socket structure is used.
     */
    volatile os_boolean used;

    /** Flush TX ring buffer to socket as soon as possible.
     */
    volatile os_boolean flush_now;

    /** Commands to lwIP thread, set by application side, cleared by lwIP thread.
     */
    volatile os_boolean open_socket_cmd;
    volatile os_boolean close_socket_cmd;

    /** Status codes returned by lwIP thread for commands.
     */
    volatile osalStatus open_status;

    /** Current socket status code, OSAL_SUCCESS = running fine, OSAL_PENDING = waiting
        for something, other values are errors.
     */
    volatile osalStatus socket_status;

#if OSAL_MULTITHREAD_SUPPORT

    /** Events to to trig application side of socket.
     */
    osalEvent trig_app_socket;

#endif

    /** TRUE for IP v6 address, FALSE for IO v4.
     */
    os_boolean is_ipv6;

    /** Host name or IP addrss.
     */
    os_char host[OSAL_IPADDR_SZ];
    os_int port_nr;

    /** Ring buffer for received data, pointer and size. Allocated when socket is
        opened, released when socket is closed.
     */
    os_char *rx_buf;
    os_short rx_buf_sz;

    /** Head and tail index. Position in buffer to which next byte is to be written.
        Range 0 ... buf_sz-1.
     */
    volatile os_short rx_head, rx_tail;

    /** Ring buffer for transmitted data, pointer and size
     */
    os_char *tx_buf;
    os_short tx_buf_sz;

    /** Head and tail index. Head is position in buffer to which next byte is to be written.
        Range 0 ... buf_sz-1. Tail is position in buffer from which next byte is to be read. Range 0 ... buf_sz-1.
     */
    volatile os_short tx_head, tx_tail;

    /** Connection identifier (PCB), OS_NULL if none.
     */
    struct tcp_pcb *socket_pcb;

    /** Buffering incoming lwip data here.
     */
    struct pbuf *incoming_buf;

    /** Data is moved here from incoming for processing into ring buffer.
     */
    struct pbuf *current_buf;

    /** Current position in current_buf.
     */
    os_int current_pos;


#if 0
    /* Events for triggering send and select.
     */
    osalEvent select_event;
#endif

}
osalSocket;


/** Arduino specific socket class to store information.
 */
typedef struct osalLWIPThread
{
    /** Network interface configuration.
     */
    osalNetworkInterfaceOld nic[OSAL_MAX_NRO_NICS];
    os_int n_nics;

#if OSAL_MULTITHREAD_SUPPORT

    /** Mutex for synchronizing socket structure reservation.
     */
    osalMutex socket_struct_mutex;

    /** Events to to trig LWIP to work.
     */
    osalEvent trig_lwip_thread_event;

#endif

}
osalLWIPThread;

/** Maximum number of sockets.
 */
#define OSAL_MAX_SOCKETS 4

/* Array of structures for TCP sockets.
 */
static osalSocket osal_sock[OSAL_MAX_SOCKETS];

/* LWIP thread state structure.
 */
static osalLWIPThread osal_lwip;


/* Prototypes for forward referred static functions.
 */
static osalSocket *osal_reserve_socket_struct(void);

void osal_socket_close(
    osalStream stream);

static void osal_lwip_serve_socket(
    osalSocket *w);

static osalStatus osal_lwip_connect_socket(
    osalSocket *w);

static err_t osal_lwip_connect_callback(
    void *arg,
    struct tcp_pcb *tpcb,
    err_t err);

static osalStatus osal_lwip_close_socket(
    osalSocket *w);

static err_t osal_lwip_data_received_callback(
    void *arg,
    struct tcp_pcb *tpcb,
    struct pbuf *p,
    err_t err);

static void osal_lwip_move_received_data_to_ring_buffer(
    osalSocket *w);

static err_t osal_lwip_ready_to_send_callback(
    void *arg,
    struct tcp_pcb *tpcb,
    u16_t len);

static void osal_lwip_send_data_from_buffer(
    osalSocket *w);

static err_t osal_lwip_thread_accept_callback(
    void *arg,
    struct tcp_pcb *newpcb,
    err_t err);

static void osal_lwip_error_callback(
    void *arg,
    err_t err);

void osal_socket_initialize_2(
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
    osalSocket *w = OS_NULL;
    osalStatus rval = OSAL_STATUS_FAILED, s;
    os_memsz sz;

    /* Return OS_NULL if network not (yet) initialized initialized.
     */
    s = osal_are_sockets_initialized();
    if (s)
    {
        if (status) *status = s;
        return OS_NULL;
    }

    /* Get first unused osal_sock structure.
     */
    w = osal_reserve_socket_struct();
    if (w == OS_NULL)
    {
        osal_debug_error("osal_sock: Too many sockets");
        goto getout;
    }

    os_memclear(w, sizeof(osalSocket));
    w->hdr.iface = &osal_socket_iface;

    /* Get host name or numeric IP address and TCP port number from parameters.
       The host buffer must be released by calling os_free() function,
       unless if host is OS_NULL (unspecified).
     */
    osal_socket_get_ip_and_port(parameters,
        &w->port_nr, w->host, OSAL_IPADDR_SZ, &w->is_ipv6, flags, IOC_DEFAULT_SOCKET_PORT);

    /* Allocate ring buffers.
     */
    w->tx_buf = os_malloc(OSAL_SOCKET_TX_BUF_SZ, &sz);
    w->tx_buf_sz = (os_short)sz;
    w->rx_buf = os_malloc(OSAL_SOCKET_RX_BUF_SZ, &sz);
    w->rx_buf_sz = (os_short)sz;
    if (w->tx_buf == OS_NULL || w->rx_buf == OS_NULL)
    {
        osal_debug_error("socket ring buffer allocation failed");
        goto getout;
    }

    /* Create event used by LWIP to trig the application side.
     */
    w->trig_app_socket = osal_event_create();
    osal_debug_assert(w->trig_app_socket);

    osal_trace2_str("~Connecting socket to ", w->host);
    osal_trace2_int(", port ", w->port_nr);

    /* Give open socket command to lwIP thread and wait for lwIP thread to carry it out
       set to application side, cleared by lwIP thread.
     */
    w->used = OS_TRUE;
    w->open_socket_cmd = OS_TRUE;
    osal_event_set(osal_lwip.trig_lwip_thread_event);
    while (w->open_socket_cmd)
    {
        osal_event_wait(w->trig_app_socket, OSAL_EVENT_INFINITE);
    }

    /* If open failed
     */
    if (w->open_status)
    {
        rval = w->open_status;
        goto getout;
    }

    /* Success. Set status code and return socket structure pointer
       casted to stream pointer.
     */
    if (status) *status = w->open_status;
    return (osalStream)w;

getout:
    /* Clean up all resources.
     */
    osal_socket_close((osalStream)w, OSAL_STREAM_DEFAULT);

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
    osalSocket *w;

    if (stream == OS_NULL) return;
    w = (osalSocket*)stream;
    if (!w->reserved) return;

    /* Give close socket command to lwIP thread and wait for lwIP thread to carry it out
       set to application side, cleared by lwIP thread.
     */
    w->close_socket_cmd = OS_TRUE;
    osal_event_set(osal_lwip.trig_lwip_thread_event);
    while (w->close_socket_cmd)
    {
        osal_event_wait(w->trig_app_socket, OSAL_EVENT_INFINITE);
    }

    /* Release event
     */
    osal_event_delete(w->trig_app_socket);

    /* Release ring buffers.
     */
    os_free(w->tx_buf, w->tx_buf_sz);
    os_free(w->rx_buf, w->rx_buf_sz);

    /* This structure is no longer used.
     */
    os_memclear(w, sizeof(osalSocket));
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
    if (status) *status = OSAL_STATUS_FAILED;
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
    osalSocket *w;
    osalStatus s;

    if (stream)
    {
        w = (osalSocket*)stream;
        if (!w->used) return OSAL_STATUS_FAILED;

        if (w->tx_head != w->tx_tail)
        {
            w->flush_now = OS_TRUE;
            osal_event_set(osal_lwip.trig_lwip_thread_event);
        }

        s = w->socket_status;
        if (s)
        {
            return (s == OSAL_PENDING) ? OSAL_SUCCESS : s;
        }
    }

    return OSAL_STATUS_FAILED;
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
  @param   flags Flags for the function, ignored by this implementation.
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
    osalSocket *w;
    os_int count;
    osalStatus status;
    os_char *wbuf;
    os_short head, tail, buf_sz, nexthead;
    os_int copynow, space;

    if (stream == OS_NULL)
    {
        status = OSAL_STATUS_FAILED;
        goto getout;
    }

    w = (osalSocket*)stream;
    osal_debug_assert(w->hdr.iface == &osal_socket_iface);

    if (n < 0 || buf == OS_NULL || !w->used)
    {
        status = OSAL_STATUS_FAILED;
        goto getout;
    }

    if (w->socket_status)
    {
        status = w->socket_status;
        if (status == OSAL_PENDING)
            status = OSAL_SUCCESS;
        goto getout;
    }

    if (n == 0)
    {
        status = OSAL_SUCCESS;
        goto getout;
    }

    wbuf = w->tx_buf;
    osal_debug_assert(wbuf);
    buf_sz = w->tx_buf_sz;
    head = w->tx_head;
    tail = w->tx_tail;
    count = 0;

    if (head >= tail)
    {
        copynow = n;
        space = buf_sz - head;
        if (tail == 0) space--;
        if (copynow > space) copynow = space;

        os_memcpy(wbuf + head, buf, copynow);
        head += copynow;
        if (head >= buf_sz) tail = 0;
        buf += copynow;
        n -= copynow;
        count += copynow;
    }

    if (head + 1 < tail && n)
    {
        copynow = n;
        space = tail - head - 1;
        if (tail == 0) space--;
        if (copynow > space) copynow = space;

        os_memcpy(wbuf + head, buf, copynow);
        head += copynow;
        count += copynow;
    }

    w->tx_head = head;

    /* If buffer full, flush now.
     */
    nexthead = head + 1;
    if (nexthead >= buf_sz) nexthead = 0;
    if (nexthead == tail)
    {
        w->flush_now = OS_TRUE;
        osal_event_set(osal_lwip.trig_lwip_thread_event);
    }

    *n_written = count;
    return OSAL_SUCCESS;

getout:
    *n_written = 0;
    return status;
}


/**
****************************************************************************************************

  @brief Read data from socket.
  @anchor osal_socket_read

  The osal_socket_read() function reads up to n bytes of data from socket into buffer.

  Internally this copies up to n bytes from ring buffer, which holds incoming data from lwip
  thread. If some data is moved from ring buffer, the lwip thread event is triggered. This
  allows it to move data from lwip buffers to ring buffer, if any.

  @param   stream Stream pointer representing the socket.
  @param   buf Pointer to buffer to read into.
  @param   n Maximum number of bytes to read. The data buffer must large enough to hold
           at least this many bytes.
  @param   n_read Pointer to integer into which the function stores the number of bytes read,
           which may be less than n if there are fewer bytes available. If the function fails
           n_read is set to zero.
  @param   flags Flags for the function, use OSAL_STREAM_DEFAULT (0) for default operation.
           Ignored by this implementation.

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
    osalSocket *w;
    os_int count;
    osalStatus status;
    os_char *rbuf;
    os_short head, tail, buf_sz, copynow;

    if (stream == OS_NULL)
    {
        status = OSAL_STATUS_FAILED;
        goto getout;
    }

    w = (osalSocket*)stream;
    osal_debug_assert(w->hdr.iface == &osal_socket_iface);

    if (n < 0 || buf == OS_NULL || !w->used)
    {
        status = OSAL_STATUS_FAILED;
        goto getout;
    }

    if (w->socket_status)
    {
        status = w->socket_status;
        if (status == OSAL_PENDING)
            status = OSAL_SUCCESS;
        goto getout;
    }

    if (n == 0)
    {
        status = OSAL_SUCCESS;
        goto getout;
    }

    rbuf = w->rx_buf;
    buf_sz = w->rx_buf_sz;
    head = w->rx_head;
    tail = w->rx_tail;
    count = 0;

    if (tail > head)
    {
        copynow = buf_sz - tail;
        if (copynow > n) copynow = n;

        os_memcpy(buf, rbuf + tail, copynow);
        tail += copynow;
        if (tail >= buf_sz) tail = 0;
        buf += copynow;
        n -= copynow;
        count += copynow;
    }

    if (tail < head && n)
    {
        copynow = head - tail;
        if (copynow > n) copynow = n;

        os_memcpy(buf, rbuf + tail, copynow);
        tail += copynow;
        count += copynow;
    }

    w->rx_tail = tail;
    if (count)
    {
        osal_event_set(osal_lwip.trig_lwip_thread_event);
    }

    *n_read = count;
    return OSAL_SUCCESS;

getout:
    *n_read = 0;
    return status;
}


/**
****************************************************************************************************

  @brief Get first unreserved socket state structure.
  @anchor osal_reserve_socket_struct

  The osal_reserve_socket_struct() function finds index of first unreserved item in
  osal_sock array.

  This function is thread safe and can be called from both lwip thread and application
  side threads.

  @return Pointer to socket data structure, or OS_NULL if no free ones.

****************************************************************************************************
*/
static osalSocket *osal_reserve_socket_struct(void)
{
    osalSocket *w;
    os_int i;

    osal_mutex_lock(osal_lwip.socket_struct_mutex);

    for (i = 0; i < OSAL_MAX_SOCKETS; i++)
    {
        w = osal_sock + i;
        if (!w->reserved)
        {
            w = osal_sock + i;
            w->reserved = OS_TRUE;
        }
        osal_mutex_unlock(osal_lwip.socket_struct_mutex);
        return w;
    }

    osal_mutex_unlock(osal_lwip.socket_struct_mutex);
    return OS_NULL;
}


#if OSAL_MULTITHREAD_SUPPORT
/**
****************************************************************************************************

  @brief lwIP thread.

  The osal_socket_lwip_thread is thread function which runs LWIP in multithread environment.
  The LWIP can be called only from this thread.

  @param   prm Pointer to worker thread parameter structure, not used, always OS_NULL.
  @param   done Event to set when worker thread has started.

  @return  None.

****************************************************************************************************
*/
static void osal_socket_lwip_thread(
    void *prm,
    osalEvent done)
{
    osalSocket *w;
    os_int i;

    osal_event_set(done);

    while (OS_TRUE)
    {
        osal_event_wait(osal_lwip.trig_lwip_thread_event, OSAL_EVENT_INFINITE);
        osal_are_sockets_initialized();

        for (i = 0; i < OSAL_MAX_SOCKETS; i++)
        {
            w = osal_sock + i;
            if (w->used)
            {
                osal_lwip_serve_socket(w);
            }
        }
    }
}
#endif


/**
****************************************************************************************************

  @brief lwIP thread handling of one socket (lwip thread).

  The osal_lwip_serve_socket function serves one socket.


  Failed socket close: The osal_lwip_close_socket() function can fail if we are out of memory.
  In this case we will try to close it repeatedly again until successful. To do this we
  leave close command active and retrigger the lwip thread event.

  @param   w Socket structure pointer.
  @return  None.

****************************************************************************************************
*/
static void osal_lwip_serve_socket(
    osalSocket *w)
{
    if (w->open_socket_cmd)
    {
        w->open_status = osal_lwip_connect_socket(w);
        w->socket_status = w->open_status ? w->open_status : OSAL_PENDING;
        w->open_socket_cmd = OS_FALSE;
        osal_event_set(w->trig_app_socket);
    }

    else if (w->close_socket_cmd)
    {
        if (osal_lwip_close_socket(w) == OSAL_SUCCESS)
        {
            w->used = OS_FALSE;
            w->close_socket_cmd = OS_FALSE;
            osal_event_set(w->trig_app_socket);
        }
        else
        {
            osal_event_set(osal_lwip.trig_lwip_thread_event);
        }
    }

    else
    {
        osal_lwip_move_received_data_to_ring_buffer(w);

        osal_lwip_send_data_from_buffer(w);
    }
}



/**
****************************************************************************************************

  @brief Start connecting a socket  (lwip thread).

  The osal_lwip_connect_socket function initiates socket connection. This function doesn't wait
  for connect, osal_lwip_connect_callback is for that.

  @param   w Socket structure pointer.
  @return  OSAL_SUCCESS if connection was successfully initiated. OSAL_PENDING indicates
           that we are waiting for network initialization (WiFi, etc.) to complete.

****************************************************************************************************
*/
static osalStatus osal_lwip_connect_socket(
    osalSocket *w)
{
    struct tcp_pcb *tpcb;
    os_uchar ipbytes[16];
    ip_addr_t ip4;
    err_t err;

    if (!osal_wifi_initialized) return OSAL_PENDING;
    osal_trace2("lwip_connect_socket");

    /* Convert IP address from string to binary
     */
    switch (osal_ip_from_str(ipbytes, sizeof(ipbytes), w->host))
    {
        case OSAL_SUCCESS:
            IP_ADDR4(&ip4, ipbytes[0], ipbytes[1], ipbytes[2], ipbytes[3]);
            break;

        // case OSAL_IS_IPV6:
            /* not implemented */

        default:
            return OSAL_STATUS_FAILED;
    }

    /* Allocate connection identifier (PCB)
     */
    osal_debug_assert(w->socket_pcb == OS_NULL);
    tpcb = tcp_new();
    w->socket_pcb = tpcb;
    if (tpcb == 0)
    {
        return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
    }
    tcp_arg(tpcb, w);

    /* Initiate connecting socket, sets callback for successful connect.
     */
    err = tcp_connect(tpcb, &ip4, (u16_t)w->port_nr, osal_lwip_connect_callback);
    if (err != ERR_OK)
    {
        memp_free(MEMP_TCP_PCB, tpcb);
        w->socket_pcb = OS_NULL;
        return OSAL_STATUS_FAILED;
    }

    tcp_nagle_disable(tpcb);

    /* Set other callback functions.
     */
    tcp_err(tpcb, osal_lwip_error_callback);
    tcp_recv(tpcb, osal_lwip_data_received_callback);
    tcp_sent(tpcb, osal_lwip_ready_to_send_callback);

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Callback when socket connection has been established (lwip thread).

  The osal_lwip_connect_callback function changes socket status from waiting for connection
  (OSAL_PENDING) to connected (OSAL_SUCCESS) ....

  @param   arg Pointer to socket structure, set by tcp_arg(w->socket_pcb, w) for the PCB.
  @param   tpcb	The connection pcb which is connected.
  @param   err	An unused error code, always ERR_OK currently
  @return  None.

****************************************************************************************************
*/
static err_t osal_lwip_connect_callback(
    void *arg,
    struct tcp_pcb *tpcb,
    err_t err)
{
    osalSocket *w;
    w = (osalSocket*)arg;
    osal_trace2("lwip_connect_callback");
    osal_debug_assert(w);

    w->socket_status = OSAL_SUCCESS;
    /* osal_event_set(w->trig_app_socket); */

    return ERR_OK;
}


/**
****************************************************************************************************

  @brief Callback when socket has failed somehow (lwip thread).

  The osal_lwip_error_callback function is called when socket connection fails (is disconnected
  for any reason). The function changes socket status to failed (OSAL_STATUS_FAILED) and
  triggers application end.

  Function also disables any future callbacks for the PCB.

  @param   arg Pointer to socket structure, set by tcp_arg(w->socket_pcb, w) for the PCB.
  @param   err Error code to indicate why the pcb has been closed ERR_ABRT: aborted through
           tcp_abort or by a TCP timer ERR_RST: the connection was reset by the remote host

  @return  None.

****************************************************************************************************
*/
static void osal_lwip_error_callback(
    void *arg,
    err_t err)
{
    osalSocket *w;
    w = (osalSocket*)arg;
    osal_trace2("lwip_error_callback");
    osal_debug_assert(w);


    w->socket_status = OSAL_STATUS_FAILED;
    osal_event_set(w->trig_app_socket);

    // if (select callback) trig
}


/**
****************************************************************************************************

  @brief Close socket (lwip thread).

  The osal_lwip_close_socket function closes socket connection and frees PCB. If closing socket
  fails, the function leaves PCB allocated, and returns error code. Thus applications
  close function will not return to try later on again. In this case of error lwip will trigger
  it's own event to keep on retrying.

  @param   w Socket structure pointer.
  @return  OSAL_SUCCESS if the connection was successfully closed. Other return values indicate 
           that closing socket failed and needs to be retried until successful.

****************************************************************************************************
*/
static osalStatus osal_lwip_close_socket(
    osalSocket *w)
{
    err_t err;
    struct tcp_pcb *tpcb;
    osal_trace2("lwip_close_socket");

    tpcb = w->socket_pcb;
    osal_debug_assert(tpcb);

    tcp_err(tpcb, 0);
    tcp_recv(tpcb, 0);
    tcp_err(tpcb, 0);
    tcp_sent(tpcb, 0);

    if (w->incoming_buf)
    {
        if (!pbuf_free(w->incoming_buf))
        {
            osal_debug_error("lwip in free failed");
        }
        w->incoming_buf = OS_NULL;
    }

    if (w->current_buf)
    {
        if (!pbuf_free(w->current_buf))
        {
            osal_debug_error("lwip processing free failed");
        }
        w->current_buf = OS_NULL;
    }

    err = tcp_close(w->socket_pcb);
    if (err != ERR_OK)
    {
        osal_debug_error("Closing lwip socket failed, no memory available");
        os_timeslice();
        return OSAL_STATUS_FAILED;
    }

    w->socket_pcb = OS_NULL;
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Called when data has been received (lwip thread).

  The osal_osal_lwip_data_received_callback function...

  @param   arg Pointer to socket structure, set by tcp_arg(w->socket_pcb, w) for the PCB.
  @param   tpcb	The connection pcb which received data
  @param   p	The received data (or NULL when the connection has been closed!)
  @param   err	An error code if there has been an error receiving Only return ERR_ABRT
           if you have called tcp_abort from within the callback function!

  @return  None.

****************************************************************************************************
*/
static err_t osal_lwip_data_received_callback(
    void *arg,
    struct tcp_pcb *tpcb,
    struct pbuf *p,
    err_t err)
{
    osalSocket *w;
    w = (osalSocket*)arg;

    osal_trace2("lwip_data_received_callback");
    osal_debug_assert(w);

    /* An empty frame, close connection.
     */
    if (p == NULL)
    {
        // close socket ?
        w->socket_status = OSAL_STATUS_HANDLE_CLOSED;
        osal_event_set(w->trig_app_socket);
        return ERR_OK;
    }

    /* A non empty frame was received from client, but err is set? Should not happen.
     */
    else if (err != ERR_OK)
    {
        w->socket_status = OSAL_STATUS_FAILED;
        pbuf_free(p);
        osal_event_set(w->trig_app_socket);
        return err;
    }

    /* Store received data.
     */
    if(w->incoming_buf == OS_NULL)
    {
        w->incoming_buf = p;
    }
    else
    {
        pbuf_cat(w->incoming_buf, p);
    }

    /* Move from lwip memory format into ring buffer.
     */
    osal_lwip_move_received_data_to_ring_buffer(w);

    return ERR_OK;
}


/**
****************************************************************************************************

  @brief Called to move incoming data to ring buffer (lwip thread).

  The osal_lwip_move_received_data_to_ring_buffer function moves data from lwip buffers
  to ring buffer to transfer it to application thread.

  @param   w Socket structure pointer.
  @return  None.

****************************************************************************************************
*/
static void osal_lwip_move_received_data_to_ring_buffer(
    osalSocket *w)
{
    struct pbuf *pr;
    os_char *buf;
    os_short head, tail, buf_sz, space;
    os_int data_sz, copynow, pos, something_done;

    buf = w->rx_buf;
    buf_sz = w->rx_buf_sz;
    head = w->rx_head;
    tail = w->rx_tail;
    something_done = 0;

    while (OS_TRUE)
    {
        if (w->current_buf == NULL)
        {
            w->current_buf = w->incoming_buf;
            w->current_pos = 0;
            w->incoming_buf = NULL;
            if (w->current_buf == OS_NULL) return;
        }

        pos = w->current_pos;
        pr = w->current_buf;
        data_sz = pr->tot_len - pos;

        if (head >= tail)
        {
            copynow = data_sz;
            space = buf_sz - head;
            if (tail == 0) space--;
            if (copynow > space) copynow = space;
            if (copynow > 0)
            {
                pbuf_copy_partial(pr, buf + head, copynow, pos);
                head += copynow;
                if (head >= buf_sz) head = 0;
                pos += copynow;
                data_sz -= copynow;
                something_done += copynow;
            }
        }

        if (head + 1 < tail)
        {
            copynow = data_sz;
            space = tail - head - 1;
            if (copynow > space) copynow = space;
            if (copynow > 0)
            {
                pbuf_copy_partial(pr, buf + head, copynow, pos);
                head += copynow;
                pos += copynow;
                data_sz -= copynow;
                something_done += copynow;
            }
        }

        w->current_pos = pos;
        w->rx_head = head;

        if (data_sz <= 0)
        {
            if (!pbuf_free(pr))
            {
                osal_debug_error("lwip pr free failed");
            }
            w->current_buf = OS_NULL;
        }
        else
        {
            break;
        }
    }

    if (something_done)
    {
        /* Inform client that we have received this. CHANGE TOT_LEN TO MATCH WHAT FIT INTO RING BUFFER
         */
        tcp_recved(w->socket_pcb, something_done);

        // if (w->select_event) osal_event_set(w->select_event);
    }
}


/**
****************************************************************************************************

  @brief Called to move outgoing data from ring buffer to lwip (lwip thread).

  The osal_lwip_move_received_data_to_ring_buffer function moves data from lwip buffers
  to ring buffer to transfer it to application thread.

  @param   w Socket structure pointer.
  @return  None.

****************************************************************************************************
*/
static void osal_lwip_send_data_from_buffer(
    osalSocket *w)
{
    os_char *buf;
    os_short head, tail, buf_sz, n;
    os_uint space;
    err_t rval;

    space = tcp_sndbuf(w->socket_pcb);
    if (w->tx_head == w->tx_tail || space <= 0 || !w->flush_now)
    {
        return;
    }
    w->flush_now = OS_FALSE;

    buf = w->tx_buf;
    buf_sz = w->tx_buf_sz;
    head = w->tx_head;
    tail = w->tx_tail;

    if (head < tail)
    {
        n = buf_sz - tail;
        if (space < n) n = space;

        rval = tcp_write(w->socket_pcb, buf + tail, n, TCP_WRITE_FLAG_COPY);
        if (rval != ERR_OK)
        {
            w->socket_status = OSAL_STATUS_HANDLE_CLOSED;
            return;
        }

        space -= n;
        tail += n;
        if (tail >= buf_sz) tail = 0;
    }

    if (head > tail && space)
    {
        n = head - tail;
        if (n > space) n = space;

        rval = tcp_write(w->socket_pcb, buf + tail, n, TCP_WRITE_FLAG_COPY);
        if (rval != ERR_OK)
        {
            w->socket_status = OSAL_STATUS_HANDLE_CLOSED;
            return;
        }

        tail += n;
    }

    w->tx_tail = tail;

    if (tail != w->tx_head)
    {
        w->flush_now = OS_TRUE;
    }

    // xxx if (select_event) trig it;
}


/**
****************************************************************************************************

  @brief Called when sent data has been acknowledged by the remote side (lwip thread).

  The osal_osal_lwip_data_received_callback function gets called when there the pcb has now
  space available to send new data.... it moved data from TX ring buffer to pcb.

  @param   arg Pointer to socket structure, set by tcp_arg(w->socket_pcb, w) for the PCB.
  @param   tpcb	The connection pcb for which data has been acknowledged.
  @param   len	The amount of bytes acknowledged. Ignored here.
  @param   err	An error code if there has been an error receiving Only return ERR_ABRT
           if you have called tcp_abort from within the callback function!

  @return  Always ERR_OK.

****************************************************************************************************
*/
static err_t osal_lwip_ready_to_send_callback(
    void *arg,
    struct tcp_pcb *tpcb,
    u16_t len)
{
    osal_lwip_send_data_from_buffer((osalSocket*)arg);
    return ERR_OK;
}


/**
****************************************************************************************************

  @brief tcp_accept lwIP callback
  @anchor osal_lwip_thread_accept_callback

  The osal_lwip_thread_accept_callback() function gets called by lwIP when new connection
  is accepted.

  @param   arg Argument for the tcp_pcb connection.
  @param   newpcb Pointer on tcp_pcb struct for the newly created tcp connection
  @param   err Not used.
  @return  Error code.

****************************************************************************************************
*/
static err_t osal_lwip_thread_accept_callback(
    void *arg,
    struct tcp_pcb *newpcb,
    err_t err)
{
#if 0
    osalSocket *w;
    err_t ret_err;
    os_int i;

    w = (osalSocket*)arg;
    osalSocket *w
    if (w->accepted_w)
    {
        ret_err = ERR_MEM;
        goto getout;
    }

    find free socket port;
    if (i == NULL)
    {
        ret_err = ERR_MEM;
        goto getout;
    }

    ->accepted_es = es;
    es->pcb = newpcb;

    /* set priority for the newly accepted tcp connection newpcb */
//    tcp_setprio(newpcb, TCP_PRIO_MIN ??);

    tcp_arg(newpcb, neww);

    tcp_nagle_disable(newpcb);
    tcp_recv(newpcb, osal_lwip_thread_recv_callback);
    tcp_err(newpcb, osal_lwip_thread_error_callback);
    tcp_sent(newpcb, osal_lwip_thread_sent_callback);
#endif

    return ERR_OK;

#if 0
getout:
    /*  close tcp connection */
    close_pcb(newpcb);

    /* return error code */
    return ret_err;
#endif
}


/**
****************************************************************************************************

  @brief Initialization continues here.
  @anchor osal_socket_initialize_2

  The osal_socket_initialize_2() initializes starts LWIP listening.

  @return  None.

****************************************************************************************************
*/
void osal_socket_initialize_2(void)
{
    const os_char *wifi_net_name = "bean24";
    const os_char *wifi_net_password = "talvi333";

    /* Initialize only once.
     */
    /* IPAddress
        ip_address(192, 168, 1, 201),
        dns_address(8, 8, 8, 8),
        gateway_address(192, 168, 1, 254),
        subnet_mask(255, 255, 255, 0);

    byte
        mac[6] = {0x66, 0x7F, 0x18, 0x67, 0xA1, 0xD3};

    osal_mac_from_str(mac, osal_net_iface.mac);

     Initialize using static configuration.
    osal_ip_from_str(ip_address, osal_net_iface.ip_address);
    osal_ip_from_str(dns_address, osal_net_iface.dns_address);
    osal_ip_from_str(gateway_address, osal_net_iface.gateway_address);
    osal_ip_from_str(subnet_mask, osal_net_iface.subnet_mask);
     */

    //DO NOT TOUCH
     //  This is here to force the ESP32 to reset the WiFi and initialise correctly.
     osal_console_write("WIFI status = ");
     osal_console_write(WiFi.getMode());
     WiFi.disconnect(true);
     delay(1000);
     WiFi.mode(WIFI_STA);
     delay(1000);
     osal_console_write("WIFI status = ");
     osal_console_write(WiFi.getMode());
     // End silly stuff !!!

     WiFi.mode(WIFI_STA);
     WiFi.disconnect();
     WiFi.status() == WL_CONNECTED;
     delay(100);

    /* Start the WiFi. Do not wait for the results here, we wish to allow IO to run even
       without WiFi network.
     */
    osal_trace("Commecting to Wifi network");
    osal_trace(wifi_net_name);
    //WiFi.mode(WIFI_STA);
    //WiFi.disconnect();
    WiFi.begin(wifi_net_name, wifi_net_password);

    /* Set socket library initialized flag, now waiting for wifi initialization. We do not lock
     * the code here to allow IO sequence, etc to proceed even without wifi.
     */
    osal_sockets_initialized = OS_TRUE;
    osal_wifi_initialized = OS_FALSE;
}


/**
****************************************************************************************************

  @brief Check if WiFi network is connected.
  @anchor osal_are_sockets_initialized

  Called to check if WiFi initialization has been completed and if so, the function initializes
  has been initialized and connected. Once connection is detected,
  the LWIP library is initialized.

  @return  OSAL_SUCCESS if we are connected to a wifi network.
           OSAL_PENDING If currently connecting and have not never failed to connect so far.
           OSAL_STATUS_FALED No connection, at least for now.

****************************************************************************************************
*/
osalStatus osal_are_sockets_initialized(
    void)
{
    if (!osal_sockets_initialized) return OS_FALSE;
    if (!osal_wifi_initialized)
    {
        /* If WiFi is not connected, just return failure.
         */
        if (WiFi.status() != WL_CONNECTED)
        {
            if (os_has_elapsed(&osal_wifi_init_timer, 2000))
            {
                osal_trace2("Waiting for wifi");
                os_get_timer(&osal_wifi_init_timer);
            }
            return OSAL_STATUS_FALED;
        }
        osal_trace("Wifi network connected");

        /* Initialize LWIP library
         */
        // lwip_init();

        /* Mark that Wifi is intialized.
         */
        osal_wifi_initialized = OS_TRUE;
    }
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Initialize sockets LWIP/WizNet.
  @anchor osal_socket_initialize

  The osal_socket_initialize() function:
  - clears all static memory used by the socket wrapper and
  - saved network interface configuration.
  - Multithread mode: Create osal_socket_struct_mutex for synchronizing socket structure reservation.
  - Multithread mode: Create trig_lwip_thread_event to trig LWIP thread to action
  - Multithread mode: start lwIP thread.
  - Single thread mode: initialize lwIP library and start connecing Wifi.

  @param   nic Pointer to array of network interface structures. Can be OS_NULL to use default.
  @param   n_nics Number of network interfaces in nic array.
  @param   wifi Pointer to array of WiFi network structures. This contains wifi network name (SSID)
           and password (pre shared key) pairs. Can be OS_NULL if there is no WiFi.
  @param   n_wifi Number of wifi networks network interfaces in wifi array.
  @return  None.

****************************************************************************************************
*/
void osal_socket_initialize(
    osalNetworkInterface *nic,
    os_int n_nics,
    osalWifiNetwork *wifi,
    os_int n_wifi)
{
    osalThreadOptParams opt;

    os_memclear(&osal_lwip, sizeof(osal_lwip));
    os_memclear(osal_sock, sizeof(osal_sock));

    osal_sockets_initialized = OS_FALSE;
    osal_wifi_initialized = OS_FALSE;

    if (n_nics > OSAL_MAX_NRO_NICS) n_nics = OSAL_MAX_NRO_NICS;
    os_memcpy(&osal_lwip.nic, nic, n_nics*sizeof(osalNetworkInterfaceOld));
    osal_lwip.n_nics = n_nics;

    /* Defaults for testing, remove
     */
    if (osal_lwip.nic[0].wifi_net_name[0] == '\0')
        os_strncpy(osal_lwip.nic[0].wifi_net_name, "bean24", OSAL_WIFI_PRM_SZ);
    if (osal_lwip.nic[0].wifi_net_password[0] == '\0')
        os_strncpy(osal_lwip.nic[0].wifi_net_password, "talvi333", OSAL_WIFI_PRM_SZ);

#if OSAL_MULTITHREAD_SUPPORT
    osal_lwip.socket_struct_mutex = osal_mutex_create();
    osal_debug_assert(osal_lwip.socket_struct_mutex);
    osal_lwip.trig_lwip_thread_event = osal_event_create();
    osal_debug_assert(osal_lwip.trig_lwip_thread_event);

    osal_socket_initialize_2();

    os_memclear(&opt, sizeof(opt));
    opt.thread_name = "lwip_thread";
    opt.stack_size = OSAL_THREAD_NORMAL_STACK;
    opt.pin_to_core = OS_TRUE;
    opt.pin_to_core_nr = 0;

    osal_thread_create(osal_socket_lwip_thread,
        OS_NULL, *opt, OSAL_THREAD_DETACHED);
#else
    osal_socket_initialize_2();
#endif
    osal_global->sockets_shutdown_func = osal_socket_shutdown;
}


/**
****************************************************************************************************

  @brief Shut down sockets.
  @anchor osal_socket_shutdown

  The osal_socket_shutdown() is not used for LWIP. Clean up is not implemented.

  @return  None.

****************************************************************************************************
*/
void osal_socket_shutdown(
    void)
{
}


#if OSAL_SOCKET_MAINTAIN_NEEDED
/**
****************************************************************************************************

  @brief Keep the sockets library alive.
  @anchor osal_socket_maintain

  The osal_socket_maintain() function is not needed for raw LWIP, empty function is here just
  to allow build if OSAL_SOCKET_MAINTAIN_NEEDED is on.

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
