/**

  @file    socket/arduino/osal_socket_lwip.c
  @brief   OSAL sockets API for ESP32/Arduino WiFi implementation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    20.1.2017

  Implementation of OSAL sockets over ESP WiFi within Arduino framework.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_SOCKET_SUPPORT==OSAL_SOCKET_LWIP_RAW

/* Force tracing on for this source file.
 */
// #undef OSAL_TRACE
// #define OSAL_TRACE 3

#include "lwip.h"
#include "lwip/netif.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/stats.h"
// #include "lwip/prot/dhcp.h"
//#include "lwip/debug.h"


/* Queue sizes.
 */
#define OSAL_SOCKET_RX_BUF_SZ 1450
#define OSAL_SOCKET_TX_BUF_SZ 1450

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
    os_boolean reserved;

    /** Nonzero if socket structure is used.
     */
    os_boolean used;

    /** Commands to lwIP thread, set by application side, cleared by lwIP thread.
     */
    os_boolean open_socket_cmd;
    os_boolean close_socket_cmd;

    /** Status codes returned by lwIP thread for commands.
     */
    osalStatus open_status;

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

#if 0
    /* Belongs to LWIP thread ??
     */
      int procpos;
      struct pbuf *proc;
    struct pbuf *in;        /* pointer on the received pbuf */
      struct tcp_pcb *pcb;    /* pointer on the current tcp_pcb */

    /* Worker thread handle and exit status code.
     */
    osalThreadHandle *thread_handle;
    osalStatus socket_status;
#endif
}
osalSocket;


/** Arduino specific socket class to store information.
 */
typedef struct osalLWIPThread
{
    /** Network interface configuration.
     */
    osalNetworkInterface nic[OSAL_DEFAULT_NRO_NICS];
    os_int n_nics

#if OSAL_MULTITHREAD_SUPPORT

    /** Mutex for synchronizing socket structure reservation.
     */
    osalMutex socket_struct_mutex;

    /** Events to to trig LWIP to work.
     */
    osalEvent trig_lwip_thread_event;

#endif


#if 0

      struct pbuf *in;        /* pointer on the received pbuf */
      struct pbuf *processing;
      struct tcp_pcb *pcb;    /* pointer on the current tcp_pcb */
      int procpos;

    /* TRUE for IP v6 address, FALSE for IO v4.
     */
    os_boolean is_ipv6;

    /* Worker thread handle and exit status code.
     */
    osalThreadHandle *thread_handle;
    osalStatus worker_status;

    /* Events for triggering send and select.
     */
    osalEvent select_event;
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

void osal_socket_esp_close(
    osalStream stream);

static void osal_socket_esp_client(
    void *prm,
    osalEvent done);


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
    osalSocket *w = OS_NULL;
    osalStatus rval = OSAL_STATUS_FAILED;
    os_memsz sz;

    /* Initialize sockets library, if not already initialized.
     */
    if (!osal_sockets_initialized)
    {
        osal_socket_initialize(OS_NULL, 0);
    }

    /* If WiFi network is not connected, we can do nothing.
     */
    if (!osal_is_wifi_initialized())
    {
        rval = OSAL_STATUS_PENDING;
        goto getout;
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
       unless if host is OS_NULL (unpecified).
     */
    osal_socket_get_host_name_and_port(parameters,
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
        osal_event_wait(w->trig_app_socket);
    }

    /* Success. Set status code and return socket structure pointer
       casted to stream pointer.
     */
    if (status) *status = OSAL_SUCCESS;
    return (osalStream)w;

getout:
    /* Clean up all resources.
     */
    osal_socket_esp_close((osalStream)w);

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
    if (!w->reserved) return;

    /* Give close socket command to lwIP thread and wait for lwIP thread to carry it out
       set to application side, cleared by lwIP thread.
     */
    w->close_socket_cmd = OS_TRUE;
    osal_event_set(osal_lwip.trig_lwip_thread_event);
    while (w->close_socket_cmd)
    {
        osal_event_wait(w->trig_app_socket);
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
    osalSocket *w;

    if (stream)
    {
        w = (osalSocket*)stream;
        if (!w->used) return OSAL_STATUS_FAILED;

        if (w->worker_status)
        {
            return w->worker_status;
        }

        if (w->tx_head |= w->tx_tail)
        {
            osal_event_set(w->tx_event);
        }
    }

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
    os_int count;
    osalStatus status;
    os_char *wbuf;
    os_short head, tail, buf_sz, nexthead, buffered_n;

    if (stream)
    {
        w = (osalSocket*)stream;
        osal_debug_assert(w->hdr.iface == &osal_socket_iface);

        if (n < 0 || buf == OS_NULL || !w->used)
        {
            status = OSAL_STATUS_FAILED;
            goto getout;
        }

        if (w->worker_status)
        {
            status = w->worker_status;
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

        while (n > 0)
        {
            nexthead = head + 1;
            if (nexthead >= buf_sz) nexthead = 0;
            if (nexthead == tail) break;
            wbuf[head] = *(buf++);
            head = nexthead;
            n--;
            count++;
        }

        w->tx_head = head;
        buffered_n = head - tail;
        if (buffered_n < 0) buffered_n += buf_sz;
        if (buffered_n > 0) // 2*buf_sz/3) // ?????????????????????????????????????????
        {
            osal_event_set(w->tx_event);
        }

        *n_written = count;
        return OSAL_SUCCESS;
    }
    status = OSAL_STATUS_FAILED;

getout:
    *n_written = 0;
    return status;
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
    os_int count;
    osalStatus status;
    os_char *rbuf;
    os_short head, tail, buf_sz;

    if (stream)
    {
        w = (osalSocket*)stream;
        osal_debug_assert(w->hdr.iface == &osal_socket_iface);

        if (n < 0 || buf == OS_NULL || !w->used)
        {
osal_trace("here");
            status = OSAL_STATUS_FAILED;
            goto getout;
        }

        if (w->worker_status)
        {
            status = w->worker_status;
            goto getout;
        }

        if (n == 0)
        {
            status = OSAL_SUCCESS;
            goto getout;
        }

        rbuf = w->rx_buf;
        osal_debug_assert(rbuf);
        buf_sz = w->rx_buf_sz;
        head = w->rx_head;
        tail = w->rx_tail;
        count = 0;

        while (n > 0 && tail != head)
        {
            *(buf++) = rbuf[tail];
            n--;
            count++;

            if (++tail >= buf_sz) tail = 0;
        }

        w->rx_tail = tail;

        *n_read = count;
        return OSAL_SUCCESS;
    }
    status = OSAL_STATUS_FAILED;

getout:
    *n_read = 0;
    return status;
}


static osalSocket *osal_get_socket_struct(
    AsyncClient *c)
{
    int i;

    for (i = 0; i < OSAL_MAX_SOCKETS; i++)
    {
        if (c == osal_sock[i].async_client && osal_sock[i].used)
        {
            osal_trace2("Async found");
            return osal_sock + i;
        }
    }
    return OS_NULL;
}

static void osal_quit_worker(
    AsyncClient *c,
    osalStatus s)
{
    osalSocket *w;

    osal_trace("QUIT_WORKER");

    w = osal_get_socket_struct(c);
    if (w)
    {
        w->worker_status = s;
        w->stop_worker_thread = OS_TRUE;
        osal_event_set(w->tx_event);
        if (w->select_event) osal_event_set(w->select_event);
    }
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
    int i;

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
    int i;

    osal_lwip_initialize();
    osal_event_set(done);

    while (OS_TRUE)
    {
        /* If we have no wifi connection, wait for one.
         */
        // ?? os_boolean osal_is_wifi_initialized(
            void)

        osal_event_wait(osal_lwip.trig_lwip_thread_event);

        for (i = 0; i < OSAL_MAX_SOCKETS; i++)
        {
            w = osal_socket + i;
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

  @brief lwIP thread handling of one socket.

  The osal_lwip_serve_socket function serves one socket.

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
        w->socket_status = w->open_status ? w->open_status : OSAL_STATUS_PENDING;
        w->open_socket_cmd = OS_FALSE;
        osal_event_set(w->trig_app_socket);
    }

    if (w->close_socket_cmd)
    {
        w->used = OS_FALSE;
        w->close_socket_cmd = OS_FALSE;
        osal_event_set(w->trig_app_socket);
    }
}


/**
****************************************************************************************************

  @brief Start connecting a socket.

  The osal_lwip_connect_socket function initiates socket connection. This function doesn't wait
  for connect, osal_lwip_connect_callback is for that.

  @param   w Socket structure pointer.
  @return  None.

****************************************************************************************************
*/
static osalStatus void osal_lwip_connect_socket(
    osalSocket *w)
{
    struct tcp_pcb *pcb;
    err_t err;

    pcb = tcp_new();
    if (my_pcb == 0)
    {
        return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
    }

    err = tcp_connect(pcb, struct ip_addr *ipaddr , u16_t port , osal_lwip_connect_callback);
    if (err != ERR_OK)
    {
        memp_free(MEMP_TCP_PCB, pcb);
        return OSAL_STATUS_FAILED;
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Callback when socket connection has been established or failed.

  The osal_lwip_connect_callback function....

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

    w->socket_status = connected or failed

    osal_event_set(w->trig_app_socket);
}


static void osal_lwip_thread_read(
    osalSocket *w)
{
    client.onData([](void* arg, AsyncClient * c, void* data, size_t len)
    {
        osalSocket *w;
        os_char *buf, *p;
        os_timer start_t;
        os_short head, tail, buf_sz, n, nexthead;

        w = osal_get_socket_struct(c);
        if (w == OS_NULL) return;

        // osal_trace_int("Data received, length =", len);

        p = (os_char*)data;
        n = (os_short)len;

        buf = w->rx_buf;
        buf_sz = w->rx_buf_sz;
        head = w->rx_head;
        tail = w->rx_tail;
        os_get_timer(&start_t);

        while (n > 0)
        {
            nexthead = head + 1;
            if (nexthead >= buf_sz) nexthead = 0;
            if (nexthead == tail)
            {
                w->rx_head = head;
                if (os_elapsed(&start_t, 10000))
                {
                    osal_quit_worker(c, OSAL_STATUS_TIMEOUT);
                    return;
                }
                os_timeslice();
                tail = w->rx_tail;
                continue;
            }
            buf[head] = *(p++);
            head = nexthead;
            n--;
        }
        w->rx_head = head;

        if (w->select_event) osal_event_set(w->select_event);
    });
}

static void osal_lwip_thread_write(
    osalSocket *w)
{
    os_char *buf;
    os_short head, tail, buf_sz, n_appended, n;
    os_int total_appended;

    client.connect(w->host, w->port_nr);

    buf = w->tx_buf;
    buf_sz = w->tx_buf_sz;
    total_appended = 0;

     //   osal_event_wait(w->tx_event, OSAL_EVENT_INFINITE); // ?????????????????????????????????????????

        //send();
        os_timeslice(); // ?????????????????????????????????????????
        os_sleep(10);

        if (!client.canSend())
        {
            osal_event_set(w->tx_event);
            continue;
        }

        if (w->tx_head != w->tx_tail)
        {
            tail = w->tx_tail;

            do
            {
                head = w->tx_head;

                n_appended = 0;
                total_appended = 0;
                if (head < tail)
                {
                    n = buf_sz - tail;
                    n_appended = client.space();
                    if (n_appended < n) n = n_appended;
                    if (n > 0)
                    {
                        if (client.add(buf + tail, n) != n)
                        {
                            osal_trace("client.add  failed");
                        }
                        total_appended += n;
//                        tail += n;
//                        if (!client.send())
//                        {
//                            osal_trace("client.send failed");
//                        }
                    }

                    tail += n_appended;
                    if (tail >= buf_sz) tail = 0;
                }
                if (head > tail)
                {
                    n = head - tail;
                    n_appended = client.space();
                    if (n_appended < n) n = n_appended;
                    if (n > 0)
                    {
                        if (client.add(buf + tail, n) != n)
                        {
                            osal_trace("client.add  failed");
                        }
                        total_appended += n;
                        tail += n;
//                        if (!client.send())
//                        {
//                            osal_trace("client.send failed");
//                        }
                    }
                }



                w->tx_tail = tail;
                if (w->tx_head == tail) break;

                os_timeslice();

                if (!n_appended) break;
            }
            while (w->tx_head != tail && total_appended && !w->stop_worker_thread && osal_go());

            client.send();

        }


    while (!w->stop_worker_thread && osal_go())
    {
    if (!w->worker_status)
    {
        w->worker_status = OSAL_STATUS_STREAM_CLOSED;
        w->stop_worker_thread = OS_TRUE;
        if (w->select_event) osal_event_set(w->select_event);
    }
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
    osalSocket *w;
    err_t ret_err;
    int i;

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
    tcp_poll(newpcb, osal_lwip_thread_poll_callback, 10);
    tcp_sent(newpcb, osal_lwip_thread_sent_callback);

    return ERR_OK;

getout:
    /*  close tcp connection */
    close_pcb(newpcb);

    /* return error code */
    return ret_err;
}


/**
****************************************************************************************************

  @brief tcp_recv callback
  @anchor osal_lwip_thread_error_callback

  The osal_lwip_thread_recv_callback() function gets called by lwIP when new data arrives.

  The TCP protocol specifies a window that tells the sending host how much data it can send
  on the connection. The window size for all connections is TCP_WND which may be overridden
  in lwipopts.h. When the application has processed the incoming data, it must call the
  tcp_recved() function to indicate that TCP can increase the receive window.

  @param   arg Argument for the tcp_pcb connection.
  @param   tpcb The pcb for the tcp connection.
  @param   pointer to the received pbuf.
  @param   err Error information regarding the reveived pbuf.
  @return  Error code.

****************************************************************************************************
*/
static err_t osal_lwip_thread_recv_callback(
    void *arg,
    struct tcp_pcb *tpcb,
    struct pbuf *p,
    err_t err)
{
    osalSocket *w;
    err_t ret_err;

    LWIP_ASSERT("arg != NULL",arg != NULL);

    w = (osalSocket*)arg;

    /* if we receive an empty tcp frame from client => close connection */
    if (p == NULL)
    {
        /* we're done sending, close connection */
        closes(w);
        ret_err = ERR_OK;
    }
    /* else : a non empty frame was received from client but for some reason err != ERR_OK */
    else if(err != ERR_OK)
    {
        /* free received pbuf*/
        if (p != NULL)
        {
            pbuf_free(p);
        }
        ret_err = err;
    }
    else
    {
        tcp_recved(es->pcb, p->tot_len);

        /* more data received from client and previous data has been already sent*/
        if(es->in == NULL)
        {
            es->in = p;
        }
        else
        {
            pbuf_cat(es->in, p);
        }
        ret_err = ERR_OK;
    }

    return ret_err;
}


/**
****************************************************************************************************

  @brief tcp_err lwIP callback function
  @anchor osal_lwip_thread_error_callback

  The osal_lwip_thread_error_callback() function...

  @param   arg
  @param   err Not used.
  @return  None.

****************************************************************************************************
*/
static void osal_lwip_thread_error_callback(
    void *arg,
    err_t err)
{
    osal_error("lwip error callback");
}


/**
****************************************************************************************************

  @brief tcp_poll lwIP callback function
  @anchor osal_lwip_thread_poll_callback

  The osal_lwip_thread_poll_callback() function...

  @param   arg
  @param   tpcb The pcb for the tcp connection
  @return  None.

****************************************************************************************************
*/
static osal_lwip_thread_poll_callback(
    void *arg,
    struct tcp_pcb *tpcb)
{
    return ERR_OK;
}


/**
****************************************************************************************************

  @brief Initialize sockets LWIP/WizNet.
  @anchor osal_socket_initialize

  The osal_socket_initialize() initializes the underlying sockets library. This used either DHCP,
  or statical configuration parameters.

  @return  None.

****************************************************************************************************
*/
void osal_lwip_initialize(void)
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

    osal_mac_from_str(mac, osal_net_iface.mac);

     Initialize using static configuration.
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
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
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
  @anchor osal_is_wifi_initialized

  Called to check if WiFi initialization has been completed and if so, the function initializes
  has been initialized and connected. Once connection is detected,
  the LWIP library is initialized.

  osal_wifi_initialized

  @return  OS_TRUE if we are connected to WiFi network, or OS_FALSE otherwise.

****************************************************************************************************
*/
os_boolean osal_is_wifi_initialized(
    void)
{
    if (!osal_sockets_initialized) return OS_FALSE;
    if (!osal_wifi_initialized)
    {
        /* If WiFi is not connected, just return failure.
         */
        if (WiFi.status() != WL_CONNECTED)
        {
            if (os_elapsed(&osal_wifi_init_timer, 2000))
            {
                osal_trace2("Waiting for wifi");
                os_get_timer(&osal_wifi_init_timer);
            }
            return OS_FALSE;
        }
        osal_trace("Wifi network connected");

        /* Initialize LWIP library
         */
        lwip_init();

        /* Mark that Wifi is intialized.
         */
        osal_wifi_initialized = OS_TRUE;
    }
    return OS_TRUE;
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
  @return  None.

****************************************************************************************************
*/
void osal_socket_initialize(
    osalNetworkInterface *nic,
    os_int n_nics)
{
    os_memclear(osal_lwip, sizeof(osal_lwip));
    os_memclear(osal_sock, sizeof(osal_sock));

    osal_sockets_initialized = OS_FALSE;
    osal_wifi_initialized = OS_FALSE;

    if (n_nics > OSAL_DEFAULT_NRO_NICS) n_nics = OSAL_DEFAULT_NRO_NICS;
    os_memcpy(&osal_lwip.nic, nic, n_nics*sizeof(osalNetworkInterface));
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

    osal_thread_create(osal_socket_lwip_thread,
        OS_NULL, OSAL_THREAD_DETACHED, 8000, "lwip_thread");
#else
    osal_lwip_initialize();
#endif
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


#if OSAL_FUNCTION_POINTER_SUPPORT

/** Stream interface for OSAL sockets. This is structure osalStreamInterface filled with
    function pointers to OSAL sockets implementation.
 */
const osalStreamInterface osal_socket_iface
 = {osal_socket_esp_open,
    osal_socket_esp_close,
    osal_socket_esp_accept,
    osal_socket_esp_flush,
    osal_stream_default_seek,
    osal_socket_esp_write,
    osal_socket_esp_read,
    osal_stream_default_write_value,
    osal_stream_default_read_value,
    osal_stream_default_get_parameter,
    osal_stream_default_set_parameter,
    OS_NULL};

#endif

#endif
