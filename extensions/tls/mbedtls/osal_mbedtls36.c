/**

  @file    tls/common/osal_mbedtls.c
  @brief   OSAL stream API layer to use secure Mbed TLS sockets.
  @author  Pekka Lehtikoski
  @version 1.1
  @date    26.4.2021

  Secure network connectivity. Implementation of OSAL stream API and general network functionality
  using Mbed TLS.

  We do NOT want to use socket API trough mbedtls_net_* functions, but osal_stream_* api functions
  instead. This allows use of features implemented for socket stream transport wrapper.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_TLS_SUPPORT==OSAL_TLS_MBED_WRAPPER
#include "extensions/tls/mbedtls/osal_mbedtls36.h"


/** MbedTLS specific socket data structure. OSAL functions cast their own stream structure
    pointers to osalStream pointers.
 */
typedef struct osalTlsSocket
{
    /** A stream structure must start with stream header, common to all streams.
     */
    osalStreamHeader hdr;

    /** Pointer to TCP socket handle structure.
     */
    osalStream tcpsocket;

    /** Flags which were given to osal_mbedtls_open() or osal_mbedtls_accept() function.
     */
    os_int open_flags;

    /** Remote peer is connected and needs to be notified on close.
     */
    os_boolean peer_connected;

    /** Flag indicating that handshake has failed.
     */
    os_boolean handshake_failed;

    /* Both client and server
     */
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
}
osalTlsSocket;



/* Prototypes for forward referred static functions.
 */
static int osal_verify_certificate_callback(
    void *data,
    mbedtls_x509_crt *crt,
    int depth,
    uint32_t *flags);

static void osal_mbedtls_close(
    osalStream stream,
    os_int flags);

static osalStatus osal_mbedtls_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags);

static void osal_mbedtls_init(
    osalSecurityConfig *prm);

static osalStatus osal_mbedtls_setup_cert_or_key(
    mbedtls_x509_crt *cert,
    mbedtls_pk_context *pkey,
    osPersistentBlockNr default_block_nr,
    const os_char *certs_dir,
    const os_char *file_name);

static osalStatus osal_report_load_error(
    osalStatus s,
    osPersistentBlockNr block_nr,
    const os_char *file_name);

static int osal_net_recv(
    void *ctx,
    unsigned char *buf,
    size_t len);

static int osal_net_send(
    void *ctx,
    const unsigned char *buf,
    size_t len);

static osalStatus osal_mbedtls_handshake(
    osalTlsSocket *so);

static void osal_mbedtls_debug(
    void *ctx,
    int level,
    const char *file,
    int line,
    const char *str);

#if OSAL_PROCESS_CLEANUP_SUPPORT
static void osal_mbedtls_cleanup(void);

static void osal_tls_shutdown(
    void);
#endif


/**
****************************************************************************************************

  @brief Open a socket.
  @anchor osal_mbedtls_open

  The osal_mbedtls_open() function opens a TLS socket. The socket can be either listening TCP
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
          - OSAL_STREAM_NO_SELECT: Open socket without select functionality.
          - OSAL_STREAM_SELECT: Open serial with select functionality.
          - OSAL_STREAM_TCP_NODELAY: Disable Nagle's algorithm on TCP socket. Use TCP_CORK on
            linux, or TCP_NODELAY toggling on windows. If this flag is set, osal_mbedtls_flush()
            must be called to actually transfer data.
          - OSAL_STREAM_NO_REUSEADDR: Disable reusability of the socket descriptor.

          See @ref osalStreamFlags "Flags for Stream Functions" for full list of stream flags.

  @return Stream pointer representing the socket, or OS_NULL if the function failed.

****************************************************************************************************
*/
static osalStream osal_mbedtls_open(
    const os_char *parameters,
    void *option,
    osalStatus *status,
    os_int flags)
{
    osalStream tcpsocket = OS_NULL;
    osalTlsSocket *so = OS_NULL;
    os_char hostbuf[OSAL_HOST_BUF_SZ];
    os_int ret;
    osalStatus s;

    osalTLS *t;
    t = osal_global->tls;

    /* If not initialized.
     */
    if (t == OS_NULL)
    {
        if (status) *status = OSAL_STATUS_FAILED;
        return OS_NULL;
    }

    /* If WiFi network is not connected, we can do nothing.
     */
    if (osal_are_sockets_initialized())
    {
        if (status) *status = OSAL_PENDING;
        return OS_NULL;
    }

    /* Connect or listen socket. Make sure to use TLS default port if unspecified.
     */
    osal_socket_embed_default_port(parameters,
        hostbuf, sizeof(hostbuf), IOC_DEFAULT_TLS_PORT);
    tcpsocket = osal_stream_open(OSAL_SOCKET_IFACE, hostbuf, option, status, flags);
    if (tcpsocket == OS_NULL) return OS_NULL;

    /* Allocate and initialize own socket structure.
     */
    so = (osalTlsSocket*)os_malloc(sizeof(osalTlsSocket), OS_NULL);
    if (so == OS_NULL)
    {
        if (status) *status = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        return OS_NULL;
    }
    os_memclear(so, sizeof(osalTlsSocket));
    so->hdr.iface = &osal_tls_iface;
    so->open_flags = flags;
    so->tcpsocket = tcpsocket;
    mbedtls_ssl_init(&so->ssl);
    mbedtls_ssl_config_init(&so->conf);

    /* If we are connecting socket.
     */
    if ((flags & (OSAL_STREAM_LISTEN|OSAL_STREAM_CONNECT)) == OSAL_STREAM_CONNECT)
    {
        /* Initialize TLS related structures.
         */
        ret = mbedtls_ssl_config_defaults(&so->conf, MBEDTLS_SSL_IS_CLIENT,
            MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
        if (ret)
        {
            osal_debug_error_int("mbedtls_ssl_config_defaults returned ", ret);
            goto getout;
        }

        mbedtls_ssl_conf_verify(&so->conf, osal_verify_certificate_callback, so);


        /* We must use MBEDTLS_SSL_VERIFY_OPTIONAL if we have no certificate chain. This allows
           transfer of certificate chain from server to the device, effectively stamping device
           as part of IO device network.
         */
        mbedtls_ssl_conf_authmode(&so->conf,
            osal_get_network_state_int(OSAL_NS_NO_CERT_CHAIN, 0)
            ? MBEDTLS_SSL_VERIFY_NONE : MBEDTLS_SSL_VERIFY_OPTIONAL); //  MBEDTLS_SSL_VERIFY_REQUIRED);

        osal_debug_error(osal_get_network_state_int(OSAL_NS_NO_CERT_CHAIN, 0)
            ? " NO CERT CHAIN": "certificate chain loaded");

        mbedtls_ssl_conf_ca_chain(&so->conf, &t->cacert, NULL);
        mbedtls_ssl_conf_rng(&so->conf, mbedtls_ctr_drbg_random, &t->ctr_drbg);
        mbedtls_ssl_conf_dbg(&so->conf, osal_mbedtls_debug, stdout);

        ret = mbedtls_ssl_setup(&so->ssl, &so->conf);
        if (ret)
        {
            osal_debug_error_int("mbedtls_ssl_setup returned ", ret);
            goto getout;
        }

        /* We use osal socket implementation for reads and writes.
         */
        mbedtls_ssl_set_bio(&so->ssl, so, osal_net_send, osal_net_recv, NULL);

        s = osal_mbedtls_handshake(so);
        if (OSAL_IS_ERROR(s))
        {
            osal_debug_error_int("first osal_mbedtls_handshake failed, status = ", s);
            goto getout;
        }
    }

    /* Success: Set status code and cast socket structure pointer to stream
       pointer and return it.
     */
    if (status) *status = OSAL_SUCCESS;
    return (osalStream)so;

getout:
    osal_mbedtls_close((osalStream)so, 0);
    if (status) *status = OSAL_STATUS_FAILED;
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Make certificates valid forever.
  @anchor osal_mbedtls_close

  The osal_verify_certificate_callback() function clears MBEDTLS_X509_BADCERT_EXPIRED flag.
  This disables checking server certificate's expiration date. Reason is that is is often very
  difficult in embedded systems to renew certificates reliably, and we do not want out automation
  system to crash and burn at specific date.

  @param   stream Stream pointer representing the socket. After this call stream pointer will
           point to invalid memory location.
  @return  None.

****************************************************************************************************
*/
static int osal_verify_certificate_callback(
    void *data, mbedtls_x509_crt *crt, int depth, uint32_t *flags)
{
#if OSAL_MICROCONTROLLER
    char buf[128];
#else
    char buf[1024];
#endif

#if OSAL_MICROCONTROLLER == 0
    osal_trace_int("Certificate verify requested for depth ", depth);
    mbedtls_x509_crt_info(buf, sizeof(buf), "  ", crt);
    osal_trace(buf);
#endif

#if OSAL_CHECK_SERVER_CERT_EXPIRATION == 0
    /* Ignore certificate expiration date.
     */
    *flags &= ~MBEDTLS_X509_BADCERT_EXPIRED;
#endif

    /* Show if certificate is formally ok
     */
    if ( ( *flags ) == 0 ) {
        osal_trace("This certificate is formally ok (not yet accepted?)");
        // Callback to add received certificate.
    }
    else
    {
        mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", *flags);
        osal_trace_str("%s\n", buf);
    }

    return 0;
}


/**
****************************************************************************************************

  @brief Close socket.
  @anchor osal_mbedtls_close

  The osal_mbedtls_close() function closes a socket, which was created by osal_mbedtls_open()
  function. All resource related to the socket are freed. Any attempt to use the socket after
  this call may result crash.

  @param   stream Stream pointer representing the socket. After this call stream pointer will
           point to invalid memory location.
  @return  None.

****************************************************************************************************
*/
static void osal_mbedtls_close(
    osalStream stream,
    os_int flags)
{
    osalTlsSocket *so;

    /* If called with NULL argument, do nothing.
     */
    if (stream == OS_NULL) return;

    /* Cast stream pointer to socket structure pointer, get operating system's socket handle.
     */
    so = (osalTlsSocket*)stream;
    osal_debug_assert(so->hdr.iface == &osal_tls_iface);

    if (so->peer_connected)
    {
        mbedtls_ssl_close_notify(&so->ssl);
    }

    mbedtls_ssl_free(&so->ssl);
    mbedtls_ssl_config_free(&so->conf);

    /* Close the socket.
     */
    osal_stream_close(so->tcpsocket, flags);

#if OSAL_DEBUG
    /* Mark the socket closed. This is used to detect if memory is accessed after it is freed.
     */
    so->hdr.iface = OS_NULL;
#endif

    /* Free memory allocated for socket structure.
     */
    os_free(so, sizeof(osalTlsSocket));
}


/**
****************************************************************************************************

  @brief Accept connection from listening socket.
  @anchor osal_mbedtls_open

  The osal_mbedtls_accept() function accepts an incoming connection from listening socket.

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
static osalStream osal_mbedtls_accept(
    osalStream stream,
    os_char *remote_ip_addr,
    os_memsz remote_ip_addr_sz,
    osalStatus *status,
    os_int flags)
{
    osalStream tcpsocket;
    osalTlsSocket *so, *newso = OS_NULL;
    osalStatus s = OSAL_STATUS_FAILED;
    osalTLS *t;
    int ret;

    /* If called with NULL argument, do nothing.
     */
    if (stream == OS_NULL) goto getout;

    so = (osalTlsSocket*)stream;
    osal_debug_assert(so->hdr.iface == &osal_tls_iface);

    if (flags == OSAL_STREAM_DEFAULT) {
        flags = so->open_flags;
    }

    /* Try to accept as normal TCP socket. If no incoming socket to accept, return.
     */
    tcpsocket = osal_stream_accept(so->tcpsocket, remote_ip_addr,
        remote_ip_addr_sz, status, flags);
    if (tcpsocket == OS_NULL)
    {
        /* Status is already set by osal_socket_accept()
         */
        return OS_NULL;
    }

// mbedtls_ssl_session_reset(&so->ssl);

    /* Allocate and clear socket structure.
     */
    newso = (osalTlsSocket*)os_malloc(sizeof(osalTlsSocket), OS_NULL);
    if (newso == OS_NULL)
    {
        osal_stream_close(tcpsocket, OSAL_STREAM_DEFAULT);
        if (status) *status = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        return OS_NULL;
    }
    os_memclear(newso, sizeof(osalTlsSocket));

    /* Save socket stucture pointer, open flags and interface pointer.
     * Set always OSAL_STREAM_LISTEN flag. We use it to decide how to do handshake.
     */
    t = osal_global->tls;
    newso->tcpsocket = tcpsocket;
    newso->open_flags = flags|OSAL_STREAM_LISTEN;
    newso->hdr.iface = &osal_tls_iface;
    // newso->peer_connected = OS_TRUE;
    mbedtls_ssl_init(&newso->ssl);
    mbedtls_ssl_config_init(&newso->conf);

    ret = mbedtls_ssl_config_defaults(&newso->conf, MBEDTLS_SSL_IS_SERVER,
        MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret)
    {
        osal_debug_error_int("mbedtls_ssl_config_defaults failed ", ret);
        goto getout;
    }

    mbedtls_ssl_conf_rng(&newso->conf, mbedtls_ctr_drbg_random, &t->ctr_drbg);
    mbedtls_ssl_conf_dbg(&newso->conf, osal_mbedtls_debug, stdout);

    mbedtls_ssl_conf_ca_chain(&newso->conf, t->srvcert.next, NULL);
    ret = mbedtls_ssl_conf_own_cert(&newso->conf, &t->srvcert, &t->pkey);
    if (ret)
    {
        osal_debug_error_int("mbedtls_ssl_conf_own_cert failed ", ret);
        goto getout;
    }

    ret = mbedtls_ssl_setup(&newso->ssl, &newso->conf);
    if (ret)
    {
        osal_debug_error_int("mbedtls_ssl_setup failed ", ret);
        goto getout;
    }

    /* We use osal socket implementation for reads and writes.
     */
    mbedtls_ssl_set_bio(&newso->ssl, newso, osal_net_send, osal_net_recv, NULL);

    s = osal_mbedtls_handshake(newso);
    if (OSAL_IS_ERROR(s)) goto getout;

    /* Success set status code and cast socket structure pointer to stream pointer
       and return it.
     */
    if (status) *status = OSAL_SUCCESS;
    return (osalStream)newso;

getout:
    /* Failed: cleanup TLS stream, set status code and return NULL pointer.
     */
    osal_mbedtls_close((osalStream)newso, 0);
    if (status) *status = s;
    return OS_NULL;
}


/* static void lws_tls_server_client_cert_verify_config(struct lws_context_creation_info *info,
        struct lws_vhost *vh)
{
    SSL_CTX_set_verify(vh->ssl_ctx, SSL_VERIFY_PEER, NULL);
    int verify_options = SSL_VERIFY_PEER;

    if (lws_check_opt(info->options, LWS_SERVER_OPTION_PEER_CERT_NOT_REQUIRED))
        verify_options = SSL_VERIFY_FAIL_IF_NO_PEER_CERT;

    SSL_CTX_set_verify(vh->ssl_ctx, verify_options, NULL);

    return 0;
}
*/

/**
****************************************************************************************************

  @brief Flush the socket.
  @anchor osal_mbedtls_flush

  The osal_mbedtls_flush() function flushes data to be written to stream.

  IMPORTANT, FLUSH MUST BE CALLED: The osal_stream_flush(<stream>, OSAL_STREAM_DEFAULT) must
  be called when select call returns even after writing or even if nothing was written, or
  periodically in in single thread mode. This is necessary even if no data was written
  previously, the socket may have previously buffered data to avoid blocking.

  @param   stream Stream pointer representing the socket.
  @param   flags See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error.

****************************************************************************************************
*/
static osalStatus osal_mbedtls_flush(
    osalStream stream,
    os_int flags)
{
    osalTlsSocket *so;
    osalStatus s;

    if (stream)
    {
        so = (osalTlsSocket*)stream;
        osal_debug_assert(so->hdr.iface == &osal_tls_iface);

        /* Flush the underlying socket buffers.
         */
        s = osal_stream_flush(so->tcpsocket, flags);
        return s;
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Write data to socket.
  @anchor osal_mbedtls_write

  The osal_mbedtls_write() function writes up to n bytes of data from buffer to socket.

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
static osalStatus osal_mbedtls_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags)
{
    osalTlsSocket *so;
    int ret;
    osalStatus s;

    /* If called with NULL argument, do nothing.
     */
    *n_written = 0;
    if (stream == OS_NULL) return OSAL_STATUS_FAILED;
    so = (osalTlsSocket*)stream;

    /* Finish handshake first.
     */
    if (!mbedtls_ssl_is_handshake_over(&so->ssl)) {
    //if (so->ssl.state != MBEDTLS_SSL_HANDSHAKE_OVER) {
        s = osal_mbedtls_handshake(so);
        if (s == OSAL_PENDING) return OSAL_SUCCESS;
        if (s) return s;
    }

    ret = mbedtls_ssl_write(&so->ssl, (os_uchar*)buf, (size_t)n);
    if (ret < 0)
    {
        if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            so->peer_connected = OS_FALSE;
            if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
            {
                osal_trace2("mbedtls_ssl_write peer closed");
                return OSAL_STATUS_STREAM_CLOSED;
            }

            osal_trace_int("mbedtls_ssl_write failed", ret);
            return OSAL_STATUS_FAILED;
        }
        ret = 0;
    }

    *n_written = ret;
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Read data from socket.
  @anchor osal_mbedtls_read

  The osal_mbedtls_read() function reads up to n bytes of data from socket into buffer.

  @param   stream Stream pointer representing the SSL socket.
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
static osalStatus osal_mbedtls_read(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags)
{
    osalTlsSocket *so;
    int ret;
    osalStatus s;

    *n_read = 0;
    if (stream == OS_NULL) return OSAL_STATUS_FAILED;
    so = (osalTlsSocket*)stream;

    /* Finish handshake first.
     */
    if (!mbedtls_ssl_is_handshake_over(&so->ssl)) {
    // if (so->ssl.state != MBEDTLS_SSL_HANDSHAKE_OVER) {
        s = osal_mbedtls_handshake(so);
        if (s == OSAL_PENDING) return OSAL_SUCCESS;
        if (s) return s;
    }

    ret = mbedtls_ssl_read(&so->ssl, (os_uchar*)buf, (size_t)n);
    if (ret < 0)
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
            ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            so->peer_connected = OS_FALSE;
            if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
            {
                osal_trace2("mbedtls_ssl_read peer closed");
                return OSAL_STATUS_STREAM_CLOSED;
            }

            osal_trace2_int("mbedtls_ssl_read failed", ret);
            return OSAL_STATUS_FAILED;
        }
        ret = 0;
    }
    *n_read = ret;
    return OSAL_SUCCESS;
}


#if OSAL_SOCKET_SELECT_SUPPORT
/**
****************************************************************************************************

  @brief Wait for an event from one of sockets.
  @anchor osal_mbedtls_select

  The osal_mbedtls_select() function blocks execution of the calling thread until something
  happens with listed sockets, or event given as argument is triggered.

  Interrupting select: The easiest way is probably to use pipe(2) to create a pipe and add the
  read end to readfds. When the other thread wants to interrupt the select() just write a byte
  to it, then consume it afterward.

  @param   streams Array of streams to wait for. All these must be TLS sockets, different stream
           types cannot be mixed in select.
  @param   n_streams Number of stream pointers in "streams" array. Must be in range from 1 to
           OSAL_SOCKET_SELECT_MAX.
  @param   evnt Custom event to interrupt the select. OS_NULL if not needed.
  @param   timeout_ms Maximum time to wait, ms. Function will return after this time even
           there is no socket or custom event. Set OSAL_INFINITE (-1) to disable the timeout.
  @param   flags Ignored, set OSAL_STREAM_DEFAULT (0).

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error.

****************************************************************************************************
*/
static osalStatus osal_mbedtls_select(
    osalStream *streams,
    os_int nstreams,
    osalEvent evnt,
    os_int timeout_ms,
    os_int flags)
{
    osalTlsSocket *so;
    osalStream tcpstreams[OSAL_SOCKET_SELECT_MAX];
    os_int i, ntcpstreams;

    osal_debug_assert(nstreams >= 1 && nstreams <= OSAL_SOCKET_SELECT_MAX);
    ntcpstreams = 0;
    for (i = 0; i < nstreams; i++)
    {
        so = (osalTlsSocket*)streams[i];
        if (so == OS_NULL) continue;
        osal_debug_assert(so->hdr.iface == &osal_tls_iface);
        tcpstreams[ntcpstreams++] = so->tcpsocket;
    }

    return osal_stream_select(tcpstreams, ntcpstreams, evnt, timeout_ms, flags);
}
#endif



/**
****************************************************************************************************

  @brief Initialize the Mbed TLS library.
  @anchor osal_tls_initialize

  The osal_tls_initialize() initializes the underlying sockets library.

  @param   nic Pointer to array of network interface structures. Can be OS_NULL to use default.
  @param   n_nics Number of network interfaces in nic array.
  @param   wifi Pointer to array of WiFi network structures. This contains wifi network name (SSID)
           and password (pre shared key) pairs. Can be OS_NULL if there is no WiFi.
  @param   n_wifi Number of wifi networks network interfaces in wifi array.
  @param   prm Parameters related to TLS. Can OS_NULL for client if not needed.

  @return  None.

****************************************************************************************************
*/
void osal_tls_initialize(
    osalNetworkInterface *nic,
    os_int n_nics,
    osalWifiNetwork *wifi,
    os_int n_wifi,
    osalSecurityConfig *prm)
{
    if (osal_global->tls) return;

    osal_socket_initialize(nic, n_nics, wifi, n_wifi);

    osal_global->tls = (osalTLS*)os_malloc(sizeof(osalTLS), OS_NULL);
    if (osal_global->tls == OS_NULL) return;
    os_memclear(osal_global->tls, sizeof(osalTLS));

    osal_mbedtls_init(prm);
#if OSAL_PROCESS_CLEANUP_SUPPORT
    osal_global->sockets_shutdown_func = osal_tls_shutdown;
#endif
}


#if OSAL_PROCESS_CLEANUP_SUPPORT
/**
****************************************************************************************************

  @brief Shut down the Mbed TLS library.
  The osal_tls_shutdown() shuts down the underlying Mbed TLS library.

****************************************************************************************************
*/
static void osal_tls_shutdown(
    void)
{
    if (osal_global->tls == OS_NULL) return;

    osal_mbedtls_cleanup();

    os_free(osal_global->tls, sizeof(osalTLS));
    osal_global->tls = OS_NULL;

    osal_socket_shutdown();
}
#endif


/**
****************************************************************************************************

  @brief Create and initialize SSL context.
  @anchor osal_mbedtls_init

  The osal_mbedtls_init() function sets up a SSL context.

  - Initialize random number generator
  - Setup certificate authority certificate

  @return  None.

****************************************************************************************************
*/
static void osal_mbedtls_init(
    osalSecurityConfig *prm)
{
    const os_char *certs_dir;
    const os_char personalization[] = "we could collect data from IO";
    osalStatus s;
    int ret;

    osalTLS *t;
    t = osal_global->tls;

    mbedtls_ctr_drbg_init(&t->ctr_drbg);
    mbedtls_entropy_init(&t->entropy);
    ret = mbedtls_ctr_drbg_seed(&t->ctr_drbg, mbedtls_entropy_func, &t->entropy,
        (const os_uchar*)personalization, (size_t)os_strlen(personalization));
    if (ret)
    {
        osal_debug_error_int("mbedtls_ctr_drbg_seed returned ", ret);
    }

    /* If we have no path to directory containing certificates and keys, set testing default.
     */
    certs_dir = prm->certs_dir;
    if (certs_dir == OS_NULL)
    {
        certs_dir = OSAL_FS_ROOT "coderoot/eosal/extensions/tls/keys-and-certs/";
    }

    /* ************* client *************
     */
    mbedtls_x509_crt_init(&t->cacert);
    s = osal_mbedtls_setup_cert_or_key(&t->cacert, OS_NULL, OS_PBNR_CLIENT_CERT_CHAIN,
        certs_dir, prm->trusted_cert_file);

    /* Mark to network info that we need certificate chain.
     */
    if (s != OSAL_SUCCESS)  {
        osal_set_network_state_int(OSAL_NS_NO_CERT_CHAIN, 0, OS_TRUE);
    }

    /* ************* server *************
     */
    mbedtls_x509_crt_init(&t->srvcert);
    mbedtls_pk_init(&t->pkey);
    osal_mbedtls_setup_cert_or_key(&t->srvcert, OS_NULL, OS_PBNR_SERVER_CERT,
        certs_dir, prm->server_cert_file);
    osal_mbedtls_setup_cert_or_key(OS_NULL, &t->pkey, OS_PBNR_SERVER_KEY,
        certs_dir, prm->server_key_file);

    /* osal_mbedtls_setup_cert_or_key(&t->srvcert, OS_NULL, OS_PBNR_ROOT_CERT,
        certs_dir, prm->share_cert_file); */

}


/**
****************************************************************************************************

  @brief Load certificate/key from file system or persistent block.
  @anchor osal_mbedtls_setup_cert_or_key

  The osal_mbedtls_setup_cert_or_key() function reads and parses certificate from either file system
  (if we have file system support) or from persistent memory, depending on file_name. If the
  file_name is number, like "7" or empty string, for persistent storage is assumed. Otherwise
  the data is used with normail file operations.

  Note: Now files are read using Mbed TLS file operation. If needed, these can be changed
  to eosal file operations.

  @param   cert Initialized certificate structure into which to append the certificate.
  @param   default_block_nr If reading from persistent storage, this is default block
           number for the case when file name doesn't specify one.
  @param   cert_dir Directory from where certificates are read, if using file system.
  @param   file_name Specifies file name or persistent block number.
  @return  OSAL_SUCCESS if certificate or key was loaded. Other return values indicate that it
           was not loaded (missing or error).

****************************************************************************************************
*/
static osalStatus osal_mbedtls_setup_cert_or_key(
    mbedtls_x509_crt *cert,
    mbedtls_pk_context *pkey,
    osPersistentBlockNr default_block_nr,
    const os_char *certs_dir,
    const os_char *file_name)
{
    osPersistentBlockNr block_nr;
    osalStatus s;
    os_char *block;
    os_memsz block_sz;
    int ret;

#if OSAL_FILESYS_SUPPORT
    os_char path[OSAL_PATH_SZ];

    /* If we have file name which doesn't start with number, we will read from file.
     */
    if (file_name) if (!osal_char_isdigit(*file_name) && *file_name != '\0')
    {
        os_strncpy(path, certs_dir, sizeof(path));
        os_strncat(path, file_name, sizeof(path));
        if (cert)
        {
            ret = mbedtls_x509_crt_parse_file(cert, path);
            if (!ret) return OSAL_SUCCESS;
        }
        else
        {
            /* The mbedtls_pk_parse_keyfile() function is a high - level utility in the Mbed TLS
            library used to load and parse a private key from a file. It's part of the Public 
            Key (PK) abstraction layer.

            This function handles the file I/O, decodes the key from its storage format 
            (like PEM or DER), and initializes an Mbed TLS key context, making it ready 
            for cryptographic operations.
            Note: The last two parameters(f_rng, p_rng) are a random number generator(RNG) 
            callback and its context.
            These were added in a 2021 API change and are necessary for parsing some key 
            formats, such as encrypted private keys or EC keys that require computation 
            of the public key from the private one .

            Parameters :
            ctx : A pointer to an initialized mbedtls_pk_context structure. This context must 
            be empty before the call(initialized with mbedtls_pk_init() or freed with mbedtls_pk_free()) .
            path : The file path to the private key file .
            pwd : The password used to decrypt the key file, or NULL if the key is not encrypted .
            f_rng : The RNG function(e.g., mbedtls_ctr_drbg_random).
            p_rng : The RNG context(e.g., mbedtls_ctr_drbg_context).
            */
            ret = mbedtls_pk_parse_keyfile(pkey, path, 0, 0, 0);
            if (!ret) return OSAL_SUCCESS;

        }
        return osal_report_load_error(
            OSAL_STATUS_PARSING_CERT_OR_KEY_FAILED, 0, file_name);
    }
#endif

    block_nr = (osPersistentBlockNr)osal_str_to_int(file_name, OS_NULL);
    if (block_nr == 0) block_nr = default_block_nr;

    s = os_load_persistent_malloc(block_nr, &block, &block_sz);
    if (s != OSAL_SUCCESS && s != OSAL_MEMORY_ALLOCATED)
    {
        return osal_report_load_error(OSAL_STATUS_CERT_OR_KEY_NOT_AVAILABLE, block_nr, file_name);
    }

    if (cert)
    {
        ret = mbedtls_x509_crt_parse(cert, (const unsigned char *)block, (size_t)block_sz);
    }
    else
    {
        ret = mbedtls_pk_parse_key(pkey, (const unsigned char *)block, (size_t)block_sz, NULL /*pwd */, 0 /* pwdlen */, NULL, 0);
    }

    if (s == OSAL_MEMORY_ALLOCATED)
    {
        os_free(block, block_sz);
    }

    if (ret)
    {
        return osal_report_load_error(
            OSAL_STATUS_PARSING_CERT_OR_KEY_FAILED, block_nr, file_name);
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Report error while loading or parsing certificate/key (internal).
  @anchor osal_report_load_error
  @return  Status given as argument.

****************************************************************************************************
*/
static osalStatus osal_report_load_error(
    osalStatus s,
    osPersistentBlockNr block_nr,
    const os_char *file_name)
{
    os_char text[128], nbuf[OSAL_NBUF_SZ];

    os_strncpy(text, "certificate or key ", sizeof(text));

    if (block_nr)
    {
        os_strncat(text, "from presistent block ", sizeof(text));
        osal_int_to_str(nbuf, sizeof(nbuf), block_nr);
        os_strncat(text, nbuf, sizeof(text));
    }
    else
    {
        os_strncat(text, "from file ", sizeof(text));
        os_strncat(text, file_name, sizeof(text));
    }
    /* If file_name is NULL pointer, this is not even in configuration:
       Ignore load errors quietly.
     */
    if (s != OSAL_STATUS_CERT_OR_KEY_NOT_AVAILABLE || file_name != OS_NULL)
    {
        os_strncpy(text, s == OSAL_STATUS_CERT_OR_KEY_NOT_AVAILABLE
          ? ": reading failed" : ": parsing failed", sizeof(text));
        osal_error(OSAL_WARNING, eosal_mod, s, text);
        osal_set_network_state_int(OSAL_NS_SECURITY_CONF_ERROR, 0, s);
    }
#if OSAL_TRACE >= 2
    /* If we are tracing, still generate trace mark */
    else
    {
        os_strncat(text, " not loaded.", sizeof(text));
        osal_info(eosal_mod, s, text);
    }
#endif

    return s;
}


#if OSAL_PROCESS_CLEANUP_SUPPORT
/**
****************************************************************************************************

  @brief Clean up Mbed TLS library.
  @anchor osal_mbedtls_cleanup

  The osal_mbedtls_cleanup() function...

  - Free memory allocated for CA certificate.
  - Free memory allocated for random number generator and entropy.

  @param   so Stream pointer representing the SSL socket.
  @return  OSAL_SUCCESS if all fine. Other return values indicate an error.

****************************************************************************************************
*/
static void osal_mbedtls_cleanup(void)
{
    osalTLS *t;
    t = osal_global->tls;

    /* Server
     */
    mbedtls_x509_crt_free(&t->srvcert);
    mbedtls_pk_free(&t->pkey);

    /* Client
     */
    mbedtls_x509_crt_free(&t->cacert);

    mbedtls_ctr_drbg_free(&t->ctr_drbg);
    mbedtls_entropy_free(&t->entropy);
}
#endif


/**
****************************************************************************************************

  @brief Receive at most 'len' characters.
  @anchor osal_net_recv

  The osal_net_recv() function implement BIO (basic IO) receive function for Mbed TLS. This reads
  data to underlying socket trough eosal stream interface.

  @param   ctx The eosal stream for TCP socket.
  @param   buf The buffer to where to store the data.
  @param   len Size of the buffer.

  @return  The number of bytes received, or a non-zero error code. With a non-blocking
           socket, MBEDTLS_ERR_SSL_WANT_READ indicates read() would block (zero bytes read).

****************************************************************************************************
*/
static int osal_net_recv(
    void *ctx,
    unsigned char *buf,
    size_t len)
{
    os_memsz n_read;
    osalStatus s;
    osalTlsSocket *so;

    if (ctx == OS_NULL) return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
    so = (osalTlsSocket*)ctx;
    if (so->tcpsocket == OS_NULL) return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;

    s = osal_stream_read(so->tcpsocket, (os_char*)buf, len, &n_read, OSAL_STREAM_DEFAULT);
    switch (s)
    {
        case OSAL_SUCCESS:
            if (n_read == 0) return MBEDTLS_ERR_SSL_WANT_READ;
            return (int)n_read;

        case OSAL_STATUS_CONNECTION_RESET:
            return MBEDTLS_ERR_NET_CONN_RESET;

        default:
            // return MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY;
            return MBEDTLS_ERR_NET_RECV_FAILED;
    }
}


/**
****************************************************************************************************

  @brief Send at most 'len' characters.
  @anchor osal_net_send

  The osal_net_send() function implement BIO (basic IO) send function for Mbed TLS. This sends
  data to underlying socket trough eosal stream interface.

  @param   ctx The eosal stream for TCP socket.
  @param   buf Pointer to data to send.
  @param   len Size of the buffer.

  @return  The number of bytes sent, or a non-zero error code; with a non-blocking socket,
           MBEDTLS_ERR_SSL_WANT_WRITE indicates write() would block (zero bytes written).

****************************************************************************************************
*/
static int osal_net_send(
    void *ctx,
    const unsigned char *buf,
    size_t len)
{
    os_memsz n_written;
    osalStatus s;
    osalTlsSocket *so;
#if OSAL_DEBUG
    static os_boolean warning_issued = OS_FALSE;
#endif

    if (ctx == OS_NULL) return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
    so = (osalTlsSocket*)ctx;
    if (so->tcpsocket == OS_NULL) return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;

    s = osal_stream_write(so->tcpsocket, (const os_char*)buf, len,
        &n_written, OSAL_STREAM_DEFAULT);
    switch (s)
    {
        case OSAL_SUCCESS:
            if (n_written == 0)
            {
#if OSAL_DEBUG
                if (!warning_issued)
                {
                    osal_trace2("Write delayed, network busy");
                    warning_issued = OS_TRUE;
                }
#endif
                return MBEDTLS_ERR_SSL_WANT_WRITE;
            }
#if OSAL_DEBUG
            warning_issued = OS_FALSE;
#endif
            return (int)n_written;

        case OSAL_STATUS_CONNECTION_RESET:
#if OSAL_DEBUG
            warning_issued = OS_FALSE;
#endif
            return MBEDTLS_ERR_NET_CONN_RESET;

        default:
#if OSAL_DEBUG
            warning_issued = OS_FALSE;
#endif
            return MBEDTLS_ERR_NET_SEND_FAILED;
    }
}


/**
****************************************************************************************************

  @brief Do TLS handshake.
  @anchor osal_mbedtls_handshake

  We do not finish TLS handshake when a socket is opened, it would block. Instead we finish
  it in consequent read and write operations.

  @param   so TLS socket structure.
  @return  OSAL_SUCCESS if handshake has been completed, OSAL_PENDING if still working on
           handshake. Other return values indicate broken socket.

****************************************************************************************************
*/
static osalStatus osal_mbedtls_handshake(
    osalTlsSocket *so)
{
    int ret;
    uint32_t xflags;

    if (so->handshake_failed) return OSAL_STATUS_CONNECTION_REFUSED;

    /* Handshake
     */
    ret = mbedtls_ssl_handshake(&so->ssl);
    osal_stream_flush(so->tcpsocket, 0);

    if (ret && ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        osal_error(OSAL_WARNING, eosal_mod, OSAL_STATUS_CONNECTION_REFUSED, OS_NULL);
#if OSAL_DEBUG
        osal_debug_error_int("mbedtls_ssl_handshake returned ", ret);
        if (ret == MBEDTLS_ERR_MPI_ALLOC_FAILED) {
            osal_debug_error("MBEDTLS_ERR_MPI_ALLOC_FAILED");
        }
#endif
        so->handshake_failed = OS_TRUE;
        return OSAL_STATUS_CONNECTION_REFUSED;
    }

    if (!mbedtls_ssl_is_handshake_over(&so->ssl)) {
    // if (so->ssl.state != MBEDTLS_SSL_HANDSHAKE_OVER) {
        return OSAL_PENDING;
    }

    so->peer_connected = OS_TRUE;

    /* If client, verify the server certificate.
     */
    if ((so->open_flags & OSAL_STREAM_LISTEN) == 0 &&
        !osal_get_network_state_int(OSAL_NS_NO_CERT_CHAIN, 0))
    {
        xflags = mbedtls_ssl_get_verify_result(&so->ssl);
        if (xflags)
        {
            char info_text[128];

            mbedtls_x509_crt_verify_info(info_text, sizeof(info_text), "  ! ", xflags);
            osal_error(OSAL_ERROR, eosal_mod, OSAL_STATUS_SERVER_CERT_REJECTED, info_text);
            so->handshake_failed = OS_TRUE;
            return OSAL_STATUS_SERVER_CERT_REJECTED;
        }
    }

    osal_trace2("TLS handshake ok");
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Debug Mbed TLS use.
  @anchor osal_mbedtls_debug

  The osal_mbedtls_debug() function is called by Mbed TLS library to display debug outout.

  @param   ctx Ignored
  @param   level Ignored
  @param   file File name
  @param   line Line number
  @param   str Debug information text.
  @return  None.

****************************************************************************************************
*/
static void osal_mbedtls_debug(
    void *ctx,
    int level,
    const char *file,
    int line,
    const char *str)
{
#if OSAL_TRACE >= 1
    os_char text[128], nbuf[OSAL_NBUF_SZ];

    os_strncpy(text, file, sizeof(text));
    os_strncat(text, ":", sizeof(text));
    osal_int_to_str(nbuf, sizeof(nbuf), line);
    os_strncat(text, nbuf, sizeof(text));
    os_strncat(text, ": ", sizeof(text));
    os_strncat(text, str, sizeof(text));
    osal_trace(text);
#endif
}


/** Stream interface for OSAL sockets. This is structure osalStreamInterface filled with
    function pointers to OSAL sockets implementation.
 */
OS_CONST osalStreamInterface osal_tls_iface
 = {OSAL_STREAM_IFLAG_SECURE,
    osal_mbedtls_open,
    osal_mbedtls_close,
    osal_mbedtls_accept,
    osal_mbedtls_flush,
    osal_stream_default_seek,
    osal_mbedtls_write,
    osal_mbedtls_read,
#if OSAL_SOCKET_SELECT_SUPPORT
    osal_mbedtls_select,
#else
    osal_stream_default_select,
#endif
    OS_NULL,
    OS_NULL};

#endif
