/**

  @file    tls/common/osal_mbedtls.c
  @brief   OSAL stream API layer to use secure Mbed TLS sockets.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    27.1.2020

  Secure network connectivity. Implementation of OSAL stream API and general network functionality
  using Mbed TLS.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_TLS_SUPPORT==OSAL_TLS_MBED_WRAPPER

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#include <stdlib.h>
#define mbedtls_time            time
#define mbedtls_time_t          time_t
#define mbedtls_fprintf         fprintf
#define mbedtls_printf          printf
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif /* MBEDTLS_PLATFORM_C */

/* Break the build if we do not have all components.
 */
#if !defined(MBEDTLS_ENTROPY_C) || \
    !defined(MBEDTLS_SSL_TLS_C) || !defined(MBEDTLS_SSL_CLI_C) || \
    !defined(MBEDTLS_NET_C) || !defined(MBEDTLS_CTR_DRBG_C)

    *** UNABLE TO BUILD ***
    MBEDTLS_ENTROPY_C and/or
    MBEDTLS_SSL_TLS_C and/or MBEDTLS_SSL_CLI_C and/or
    MBEDTLS_NET_C and/or MBEDTLS_CTR_DRBG_C and/or not defined.
#endif

#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"


/** Mbed TLS specific global data related to TLS.
 */
typedef struct osalTLS
{
    /** Random number generator context.
     */
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;

    /** Certificate authority certificate
     */
    mbedtls_x509_crt cacert;

    /** Server only
     */
    mbedtls_x509_crt srvcert;
    mbedtls_pk_context pkey;

    /** Flag indicating the we do not have certificate chain.
     */
    os_boolean no_certificate_chain;
}
osalTLS;


/** Mbed TLS specific socket data structure. OSAL functions cast their own stream structure
    pointers to osalStream pointers.
 */
typedef struct osalTlsSocket
{
    /** A stream structure must start with stream header, common to all streams.
     */
    osalStreamHeader hdr;

    /** Flags which were given to osal_mbedtls_open() or osal_mbedtls_accept() function.
	 */
	os_int open_flags;

    /** Remote peer is connected and needs to be notified on close.
     */
    os_boolean peer_connected;

    /* Both client and server
     */
    mbedtls_net_context fd;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
}
osalTlsSocket;



/* Prototypes for forward referred static functions.
 */
static void osal_mbedtls_close(
    osalStream stream,
    os_int flags);

static osalStatus osal_mbedtls_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags);

static void my_debug(
    void *ctx,
    int level,
    const char *file,
    int line,
    const char *str );

static void osal_mbedtls_init(
    osalSecurityConfig *prm);

static osalStatus osal_mbedtls_setup_cert_or_key(
    mbedtls_x509_crt *cert,
    mbedtls_pk_context *pkey,
    osPersistentBlockNr default_block_nr,
    const os_char *certs_dir,
    const os_char *file_name);

static void osal_mbedtls_cleanup(void);


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
    osalTlsSocket *so = OS_NULL;
    os_char hostbuf[OSAL_HOST_BUF_SZ], *host, *p;
    os_char nbuf[OSAL_NBUF_SZ];
    os_int port_nr, ret;
    uint32_t xflags;

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
        if (status) *status = OSAL_STATUS_PENDING;
        return OS_NULL;
    }

    /* Separate port number and host name. Use TLS default port if unspecified.
     */
    os_strncpy(hostbuf, parameters, sizeof(hostbuf));
    port_nr = IOC_DEFAULT_TLS_PORT;
    p = os_strchr(hostbuf, ']');
    if (p == OS_NULL) p = hostbuf;
    p = os_strchr(p, ':');
    if (p) {
        *(p++) = '\0';
        port_nr = (os_int)osal_str_to_int(p, OS_NULL);
    }
    host = hostbuf[0] == '[' ? hostbuf + 1 : hostbuf;

    /* Allocate and clear own socket structure.
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

    mbedtls_net_init(&so->fd);
    mbedtls_ssl_init(&so->ssl);
    mbedtls_ssl_config_init(&so->conf);
    osal_int_to_str(nbuf, sizeof(nbuf), port_nr);

    /* Listen socket.
     */
    if (flags & OSAL_STREAM_LISTEN)
    {
        ret = mbedtls_net_bind(&so->fd, host[0] == '\0' ? NULL : host,
            nbuf, MBEDTLS_NET_PROTO_TCP );
        if (ret)
        {
            osal_debug_error_int("mbedtls_net_bind failed ", ret);
            goto getout;
        }

        ret = mbedtls_net_set_nonblock(&so->fd);
        if (ret)
        {
            osal_debug_error_int("mbedtls_net_set_nonblock failed ", ret);
            goto getout;
        }
    }

    /* Connect socket.
     */
    else
    {
        ret = mbedtls_net_connect(&so->fd, host, nbuf, MBEDTLS_NET_PROTO_TCP);
        if (ret)
        {
            osal_debug_error_int("mbedtls_net_connect returned ", ret);
            goto getout;
        }

        ret = mbedtls_net_set_nonblock(&so->fd);
        if (ret)
        {
            osal_debug_error_int("mbedtls_net_set_nonblock failed ", ret);
            goto getout;
        }

        /* Initialize TLS related structures.
         */

        if( ( ret = mbedtls_ssl_config_defaults( &so->conf,
                        MBEDTLS_SSL_IS_CLIENT,
                        MBEDTLS_SSL_TRANSPORT_STREAM,
                        MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
        {
            osal_debug_error_int("mbedtls_ssl_config_defaults returned ", ret);
            goto getout;
        }

        /* OPTIONAL is not optimal for security,
         * but makes interop easier in this simplified example */
        mbedtls_ssl_conf_authmode(&so->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
        mbedtls_ssl_conf_ca_chain(&so->conf, &t->cacert, NULL);
        mbedtls_ssl_conf_rng(&so->conf, mbedtls_ctr_drbg_random, &t->ctr_drbg);
        mbedtls_ssl_conf_dbg(&so->conf, my_debug, stdout);

        ret = mbedtls_ssl_setup(&so->ssl, &so->conf);
        if (ret)
        {
            osal_debug_error_int("mbedtls_ssl_setup returned ", ret);
            goto getout;
        }

        /* We cannot set host name for security validation, because
         * we often connect by IP address
        ret = mbedtls_ssl_set_hostname(&so->ssl, host);
        if (ret)
        {
            osal_debug_error_int("mbedtls_ssl_set_hostname returned ", ret);
            goto getout;
        } */

        //mbedtls_ssl_conf_read_timeout(&so->conf, 3000);
        //mbedtls_ssl_set_bio(&so->ssl, &so->fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

        mbedtls_ssl_set_bio(&so->ssl, &so->fd, mbedtls_net_send, mbedtls_net_recv, NULL );

        /* 4. Handshake
         */
        while (( ret = mbedtls_ssl_handshake(&so->ssl) ))
        {
            if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
            {
                osal_debug_error_int("mbedtls_ssl_handshake returned ", ret);
                goto getout;
            }
            so->peer_connected = OS_TRUE;
        }
        so->peer_connected = OS_TRUE;

        /* 5. Verify the server certificate
         */
        /* In real life, we probably want to bail out when ret != 0 */
        xflags = mbedtls_ssl_get_verify_result(&so->ssl);
        if (xflags)
        {
            char vrfy_buf[512];

            mbedtls_x509_crt_verify_info(vrfy_buf, sizeof( vrfy_buf ), "  ! ", xflags);
            osal_debug_error_str("mbedtls failed ", vrfy_buf);
        }
    }

    /* Success: Set status code and cast socket structure pointer to stream
       pointer and return it.
     */
    if (status) *status = OSAL_SUCCESS;
    return (osalStream)so;

getout:
    osal_mbedtls_close((osalStream)so, 0);

    /* mbedtls_net_free(&so->fd);
    mbedtls_ssl_free(&so->ssl);
    mbedtls_ssl_config_free(&so->conf);
    os_free(so, sizeof(osalTlsSocket)); */

    if (status) *status = OSAL_STATUS_FAILED;
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Close socket.
  @anchor osal_mbedtls_close

  The osal_mbedtls_close() function closes a socket, which was creted by osal_mbedtls_open()
  function. All resource related to the socket are freed. Any attemp to use the socket after
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

    mbedtls_net_free(&so->fd);
    mbedtls_ssl_free(&so->ssl);
    mbedtls_ssl_config_free(&so->conf);

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
		   The value OSAL_STATUS_NO_NEW_CONNECTION indicates that no new incoming 
		   connection, was accepted.  All other nonzero values indicate an error,
           See @ref osalStatus "OSAL function return codes" for full list.
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
    osalTlsSocket *so, *newso = OS_NULL;
    osalStatus s = OSAL_STATUS_FAILED;
    mbedtls_net_context client_fd;
    size_t addr_sz;
    osalTLS *t;
    os_uchar addr[16];
    int ret;

    /* If called with NULL argument, do nothing.
     */
    if (stream == OS_NULL) goto getout;


    so = (osalTlsSocket*)stream;
    osal_debug_assert(so->hdr.iface == &osal_tls_iface);

// mbedtls_ssl_session_reset(&so->ssl);
    os_memclear(&client_fd, sizeof(mbedtls_net_context));

    /* Try to accept as normal TCP socket. If no incoming socket to accept, return.
     */
    ret = mbedtls_net_accept( &so->fd, &client_fd, addr, sizeof(addr), &addr_sz);
    if (ret)
    {
        s = OSAL_STATUS_NO_NEW_CONNECTION;
        if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
            ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            osal_debug_error_int("mbedtls_net_accept failed ", ret);
            s = OSAL_STATUS_FAILED;
        }

        goto optout;
    }

    /* Convert remote IP address to string.
     */
    if (remote_ip_addr)
    {
        osal_ip_to_str(remote_ip_addr, remote_ip_addr_sz, addr, addr_sz);
    }

    /* Allocate and clear socket structure.
     */
    newso = (osalTlsSocket*)os_malloc(sizeof(osalTlsSocket), OS_NULL);
    if (newso == OS_NULL)
    {
        mbedtls_net_free(&client_fd);
        if (status) *status = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        return OS_NULL;
    }
    os_memclear(newso, sizeof(osalTlsSocket));

    /* Save socket stucture pointer, open flags and interface pointer.
     */
    t = osal_global->tls;
    newso->open_flags = flags;
    newso->hdr.iface = &osal_tls_iface;
    newso->fd = client_fd;
    newso->peer_connected = OS_TRUE;
    mbedtls_ssl_init(&newso->ssl);
    mbedtls_ssl_config_init(&newso->conf);

    ret = mbedtls_net_set_nonblock(&newso->fd);
    if (ret)
    {
        osal_debug_error_int("mbedtls_net_set_nonblock failed C ", ret);
        goto getout;
    }

    ret = mbedtls_ssl_config_defaults(&newso->conf, MBEDTLS_SSL_IS_SERVER,
        MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret)
    {
        osal_debug_error_int("mbedtls_ssl_config_defaults failed ", ret);
        goto getout;
    }

    mbedtls_ssl_conf_rng(&newso->conf, mbedtls_ctr_drbg_random, &t->ctr_drbg);
    mbedtls_ssl_conf_dbg(&newso->conf, my_debug, stdout);

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

    //mbedtls_ssl_conf_read_timeout(&newso->conf, 3000);
    //mbedtls_ssl_set_bio(&newso->ssl, &newso->fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

    mbedtls_ssl_set_bio(&newso->ssl, &newso->fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    /* 5. Handshake
     */
    while((ret = mbedtls_ssl_handshake(&newso->ssl)) != 0)
    {
        if(ret != MBEDTLS_ERR_SSL_WANT_READ &&
           ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            osal_debug_error_int("mbedtls_ssl_handshake failed ", ret);
            goto getout;
        }
    }

    /* Success set status code and cast socket structure pointer to stream pointer
       and return it.
     */
    if (status) *status = OSAL_SUCCESS;
    return (osalStream)newso;

getout:
    osal_mbedtls_close((osalStream)newso, 0);
/*
    mbedtls_net_free(&newso->fd);
    mbedtls_ssl_free(&newso->ssl);
    mbedtls_ssl_config_free(&newso->conf);
    os_free(newso, sizeof(osalTlsSocket));
*/
optout:
	/* Set status code and return NULL pointer.
	 */
    if (status) *status = s;
	return OS_NULL;
}


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
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
static osalStatus osal_mbedtls_flush(
    osalStream stream,
    os_int flags)
{
    // os_memsz n_written;
    // return osal_mbedtls_write(stream, "", 0, &n_written, 0);
    return OSAL_SUCCESS;

#if 0
    osalTlsSocket *so;
    osalStatus s;
    os_boolean work_done;

    if (stream)
    {
        so = (osalTlsSocket*)stream;
        osal_debug_assert(so->hdr.iface == &osal_tls_iface);

        /* While we have buffered data, encrypt it and move to SSL.
         */
        do
        {
            work_done = OS_FALSE;
            if (so->encrypt_len > 0)
            {
                s = osal_mbedtls_do_encrypt(so);
                switch (s)
                {
                    case OSAL_SUCCESS: work_done = OS_TRUE; break;
                    case OSAL_STATUS_NOTHING_TO_DO: break;
                    default: return s;
                }
            }
            if (so->write_len > 0)
            {
                s = osal_mbedtls_do_sock_write(so);
                switch (s)
                {
                    case OSAL_SUCCESS: work_done = OS_TRUE; break;
                    case OSAL_STATUS_NOTHING_TO_DO: break;
                    default: return s;
                }
            }
        }
        while (work_done);

        /* Flush the underlying socket buffers.
         */
        s = osal_socket_flush(so->tcpsocket, flags);
        return s;
    }
#endif
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
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

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

    /* If called with NULL argument, do nothing.
     */
    *n_written = 0;
    if (stream == OS_NULL) return OSAL_STATUS_FAILED;
    so = (osalTlsSocket*)stream;

    ret = mbedtls_ssl_write(&so->ssl, (os_uchar*)buf, n);
osal_debug_error_int("HERE X1", ret);
    if (ret < 0)
    {
        if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
            {
                so->peer_connected = OS_FALSE;
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
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

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

    *n_read = 0;
    if (stream == OS_NULL) return OSAL_STATUS_FAILED;
    so = (osalTlsSocket*)stream;

    ret = mbedtls_ssl_read(&so->ssl, (os_uchar*)buf, n);
    if (ret < 0)
    {
        // MBEDTLS_ERR_SSL_CONN_EOF
        // if (ret != MBEDTLS_ERR_SSL_TIMEOUT)
        if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
            {
                so->peer_connected = OS_FALSE;
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


#if 0
/**
****************************************************************************************************

  @brief Wait for an event from one of sockets.
  @anchor osal_mbedtls_select

  The osal_mbedtls_select() function blocks execution of the calling thread until something
  happens with listed sockets, or event given as argument is triggered.

  Interrupting select: The easiest way is probably to use pipe(2) to create a pipe and add the
  read end to readfds. When the other thread wants to interrupt the select() just write a byte
  to it, then consume it afterward.

  @param   streams Array of streams to wait for. These must be serial ports, no mixing
           of different stream types is supported.
  @param   n_streams Number of stream pointers in "streams" array. Must be in range from 1 to
           OSAL_SOCKET_SELECT_MAX.
  @param   evnt Custom event to interrupt the select. OS_NULL if not needed.
  @param   selectdata Pointer to structure to fill in with information why select call
           returned. The "stream_nr" member is stream number which triggered the return,
           or OSAL_STREAM_NR_CUSTOM_EVENT if return was triggered by custom evenet given
           as argument. The "errorcode" member is OSAL_SUCCESS if all is fine. Other
           values indicate an error (broken or closed socket, etc). The "eventflags"
           member is planned to to show reason for return. So far value of eventflags
           is not well defined and is different for different operating systems, so
           it should not be relied on.
  @param   timeout_ms Maximum time to wait in select, ms. If zero, timeout is not used.
  @param   flags Ignored, set OSAL_STREAM_DEFAULT (0).

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
           indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

  @return  None.

****************************************************************************************************
*/
static osalStatus osal_mbedtls_select(
    osalStream *streams,
    os_int nstreams,
    osalEvent evnt,
    osalSelectData *selectdata,
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

    return osal_socket_select(tcpstreams, ntcpstreams, evnt, selectdata, timeout_ms, flags);
}
#endif


static void my_debug( void *ctx, int level,
                      const char *file, int line,
                      const char *str )
{
    ((void) level);

    mbedtls_fprintf( (FILE *) ctx, "%s:%04d: %s", file, line, str );
    fflush(  (FILE *) ctx  );
}

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
}


/**
****************************************************************************************************

  @brief Shut down the Mbed TLS library.
  @anchor osal_tls_shutdown

  The osal_tls_shutdown() shuts down the underlying Mbed TLS library.

  @return  None.

****************************************************************************************************
*/
void osal_tls_shutdown(
	void)
{
    if (osal_global->tls == OS_NULL) return;

    osal_mbedtls_cleanup();

    os_free(osal_global->tls, sizeof(osalTLS));
    osal_global->tls = OS_NULL;

    osal_socket_shutdown();
}


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
        (const os_uchar*)personalization, os_strlen(personalization));
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
        certs_dir, prm->client_cert_chain_file);
    t->no_certificate_chain = (os_boolean)(s != OSAL_SUCCESS);

    /* ************* server *************
     */
    mbedtls_x509_crt_init(&t->srvcert);
    mbedtls_pk_init(&t->pkey);
    osal_mbedtls_setup_cert_or_key(&t->srvcert, OS_NULL, OS_PBNR_SERVER_CERT,
        certs_dir, prm->server_cert_file);
    osal_mbedtls_setup_cert_or_key(&t->srvcert, OS_NULL, OS_PBNR_ROOT_CERT,
        certs_dir, prm->root_cert_file);

    osal_mbedtls_setup_cert_or_key(OS_NULL, &t->pkey, OS_PBNR_SERVER_KEY,
        certs_dir, prm->server_key_file);
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
  @oaram   file_name Specifies file name or persistent block number.
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
    osalStatus s, rval;
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
            osal_debug_error_str("mbedtls_x509_crt_parse_file failed ", path);
        }
        else
        {
            ret = mbedtls_pk_parse_keyfile(pkey, path, 0);
            if (!ret) return OSAL_SUCCESS;
            osal_debug_error_str("mbedtls_pk_parse_keyfile failed ", path);
        }
        return OSAL_STATUS_FAILED;
    }
#endif

    block_nr = (osPersistentBlockNr)osal_str_to_int(file_name, OS_NULL);
    if (block_nr == 0) block_nr = default_block_nr;

    s = ioc_load_persistent_malloc(block_nr, &block, &block_sz);
    if (s != OSAL_SUCCESS && s != OSAL_STATUS_MEMORY_ALLOCATED)
    {
        osal_debug_error_int("ioc_load_persistent_malloc failed ", block_nr);
        return OSAL_STATUS_FAILED;
    }

    rval = OSAL_SUCCESS;
    if (cert)
    {
        ret = mbedtls_x509_crt_parse(cert, (const unsigned char *)block, block_sz);
        if (ret)
        {
            osal_debug_error_int("mbedtls_x509_crt_parse failed ", ret);
            rval = OSAL_STATUS_FAILED;
        }
    }
    else
    {
        ret = mbedtls_pk_parse_key(pkey, (const unsigned char *)block, block_sz, NULL, 0);
        if (ret)
        {
            osal_debug_error_int("mbedtls_pk_parse_key failed ", ret);
            rval = OSAL_STATUS_FAILED;
        }
    }

    if (s == OSAL_STATUS_MEMORY_ALLOCATED)
    {
        os_free(block, block_sz);
    }

    return rval;
}


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


/**
****************************************************************************************************

  @brief Get network and security status.
  @anchor osal_tls_get_network_status

  The osal_tls_get_network_status function retrieves network and security status information,
  like is wifi connected? Do we have client certificate chain?

  @param   net_status Network status structure to fill.
  @param   nic_nr Network interface number, ignored here since only one adapter is supported.
  @return  None.

****************************************************************************************************
*/
void osal_tls_get_network_status(
    osalNetworkStatus *net_status,
    os_int nic_nr)
{
    osalTLS *t;
    t = osal_global->tls;

    /* Get underlying socket library status (WiFi)
     */
    osal_socket_get_network_status(net_status, nic_nr);

    /* Return "no certificate chain" flag.
     */
    net_status->no_cert_chain = t->no_certificate_chain;
}


/** Stream interface for OSAL sockets. This is structure osalStreamInterface filled with
    function pointers to OSAL sockets implementation.
 */
const osalStreamInterface osal_tls_iface
 = {osal_mbedtls_open,
    osal_mbedtls_close,
    osal_mbedtls_accept,
    osal_mbedtls_flush,
	osal_stream_default_seek,
    osal_mbedtls_write,
    osal_mbedtls_read,
	osal_stream_default_write_value,
	osal_stream_default_read_value,
    osal_stream_default_get_parameter,
    osal_stream_default_set_parameter,
#if 0
    osal_mbedtls_select,
#else
    osal_stream_default_select,
#endif
    OS_TRUE}; /* is_secure */

#endif
