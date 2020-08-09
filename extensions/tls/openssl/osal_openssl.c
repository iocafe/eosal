/**

  @file    tls/common/osal_openssl.c
  @brief   OSAL stream API layer to use secure OpenSSL sockets.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Secure network connectivity. Implementation of OSAL stream API and general network functionality
  using OpenSSL. This implementation uses OSAL stream API also downwards to access underlying data
  transport socket.

  Original copyright notice and credits: This is based on example work by Mr Darren Smith.
  Original Copyright (c) 2017 Darren Smith. The example is free software; you can redistribute
  it and/or modify it under the terms of the MIT license. See LICENSE for details.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_TLS_SUPPORT==OSAL_TLS_OPENSSL_WRAPPER

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>


/** OpenSSL TLS specific global data related to TLS.
 */
typedef struct osalTLS
{
    /** Global SSL context.
     */
    SSL_CTX *ctx;
}
osalTLS;

/** This enum contols whether the SSL connection needs to initiate the SSL handshake.
 */
typedef enum osalSSLMode
{
    OSAL_SSLMODE_SERVER,
    OSAL_SSLMODE_CLIENT
}
osalSSLMode;

/** Simplified return codes.
 */
typedef enum osalSSLStatus
{
    OSAL_SSLSTATUS_OK,
    OSAL_SSLSTATUS_WANT_IO,
    OSAL_SSLSTATUS_FAIL,
}
osalSSLStatus;


#define OSAL_SSL_DEFAULT_BUF_SIZE 512
#define OSAL_ENCRYPT_BUFFER_SZ 256
#define OSAL_READ_BUF_SZ 512

/** OpenSSL specific socket data structure. OSAL functions cast their own stream structure
    pointers to osalStream pointers.
 */
typedef struct osalSSLSocket
{
    /** A stream structure must start with this generic stream header structure, which contains
        parameters common to every stream.
     */
    osalStreamHeader hdr;

    /** Pointer to TCP socket handle structure.
     */
    osalStream tcpsocket;

    /** Stream open flags. Flags which were given to osal_openssl_open() or osal_openssl_accept()
        function.
     */
    os_int open_flags;

    SSL *ssl;

    /** SSL reads from, we write to.
     */
    BIO *rbio;

    /** SSL writes to, we read from.
     */
    BIO *wbio;

    /** Bytes waiting to be written to socket. This is data that has been generated
        by the SSL object, either due to encryption of user input, or, writes
        requires due to peer-requested SSL renegotiation.
     */
    char* write_buf;
    size_t write_len;

    /** Bytes waiting to be encrypted by the SSL object.
     */
    char* encrypt_buf;
    size_t encrypt_len;

    /** Read buffer
     */
    os_char read_buf[OSAL_READ_BUF_SZ];
    os_int read_buf_n;
}
osalSSLSocket;


/* Prototypes for forward referred static functions.
 */
static void osal_openssl_init(
    osalSecurityConfig *prm);

static void osal_openssl_client_setup(
    osalSecurityConfig *prm,
    const os_char *certs_dir);

static void osal_openssl_cleanup(void);

static osalStatus osal_openssl_client_init(
    osalSSLSocket *sslsocket,
    osalSSLMode mode);

static void osal_openssl_client_cleanup(
    osalSSLSocket *sslsocket);

static osalSSLStatus osal_openssl_get_sslstatus(
    SSL* ssl,
    os_int n);

static void osal_openssl_send_unencrypted_bytes(
    osalSSLSocket *sslsocket,
    const char *buf,
    size_t len);

static void osal_openssl_queue_encrypted_bytes(
    osalSSLSocket *sslsocket,
    const char *buf,
    size_t len);

static osalSSLStatus osal_openssl_do_ssl_handshake(
    osalSSLSocket *sslsocket);

static osalStatus osal_openssl_do_encrypt(
    osalSSLSocket *sslsocket);

static osalStatus osal_openssl_do_sock_write(
    osalSSLSocket *sslsocket);

static int osal_openssl_verify_callback(
    int preverify,
    X509_STORE_CTX* x509_ctx);


/**
****************************************************************************************************

  @brief Open a socket.
  @anchor osal_openssl_open

  The osal_openssl_open() function opens a TLS socket. The socket can be either listening TCP
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
            linux, or TCP_NODELAY toggling on windows. If this flag is set, osal_openssl_flush()
            must be called to actually transfer data.
          - OSAL_STREAM_NO_REUSEADDR: Disable reusability of the socket descriptor.

          See @ref osalStreamFlags "Flags for Stream Functions" for full list of stream flags.

  @return Stream pointer representing the socket, or OS_NULL if the function failed.

****************************************************************************************************
*/
static osalStream osal_openssl_open(
    const os_char *parameters,
    void *option,
    osalStatus *status,
    os_int flags)
{
    osalStream tcpsocket = OS_NULL;
    osalSSLSocket *sslsocket = OS_NULL;
    osalStatus s;
    os_char host[OSAL_HOST_BUF_SZ];

    /* If not initialized.
     */
    if (osal_global->tls == OS_NULL)
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
        host, sizeof(host), IOC_DEFAULT_TLS_PORT);
    tcpsocket = osal_socket_open(host, option, status, flags);
    if (tcpsocket == OS_NULL) return OS_NULL;

    /* Allocate and initialize socket structure.
     */
    sslsocket = (osalSSLSocket*)os_malloc(sizeof(osalSSLSocket), OS_NULL);
    if (sslsocket == OS_NULL)
    {
        s = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        goto getout;
    }
    os_memclear(sslsocket, sizeof(osalSSLSocket));
    sslsocket->tcpsocket = tcpsocket;
    sslsocket->open_flags = flags;
    sslsocket->hdr.iface = &osal_tls_iface;

    /* If we are connecting socket.
     */
    if ((flags & (OSAL_STREAM_LISTEN|OSAL_STREAM_CONNECT)) == OSAL_STREAM_CONNECT)
    {
        /* Initialize SSL client and memory BIOs.
        */
        s = osal_openssl_client_init(sslsocket, OSAL_SSLMODE_CLIENT);
        if (s) goto getout;

        if (osal_openssl_do_ssl_handshake(sslsocket) == OSAL_SSLSTATUS_FAIL)
        {
            s = OSAL_STATUS_FAILED;
            goto getout;
        }
    }

    /* Success: Set status code and cast socket structure pointer to stream
       pointer and return it.
     */
    if (status) *status = OSAL_SUCCESS;
    return (osalStream)sslsocket;

getout:
    /* If we got far enough to allocate the socket structure.
       Close the event handle (if any) and free memory allocated
       for the socket structure.
     */
    if (sslsocket)
    {
        os_free(sslsocket, sizeof(osalSSLSocket));
    }

    /* Close socket
     */
    if (tcpsocket)
    {
        osal_socket_close(tcpsocket, OSAL_STREAM_DEFAULT);
    }

    /* Set status code and return NULL pointer.
     */
    if (status) *status = s;
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Close socket.
  @anchor osal_openssl_close

  The osal_openssl_close() function closes a socket, which was creted by osal_openssl_open()
  function. All resource related to the socket are freed. Any attemp to use the socket after
  this call may result crash.

  @param   stream Stream pointer representing the socket. After this call stream pointer will
           point to invalid memory location.
  @return  None.

****************************************************************************************************
*/
static void osal_openssl_close(
    osalStream stream,
    os_int flags)
{
    osalSSLSocket *sslsocket;

    /* If called with NULL argument, do nothing.
     */
    if (stream == OS_NULL) return;

    /* Cast stream pointer to socket structure pointer, get operating system's socket handle.
     */
    sslsocket = (osalSSLSocket*)stream;
    osal_debug_assert(sslsocket->hdr.iface == &osal_tls_iface);

    /* Clean up the OpenSSL related stuff.
     */
    osal_openssl_client_cleanup(sslsocket);

    /* Close the socket.
     */
    osal_socket_close(sslsocket->tcpsocket, flags);

#if OSAL_DEBUG
    /* Mark the socket closed. This is used to detect if memory is accessed after it is freed.
     */
    sslsocket->hdr.iface = OS_NULL;
#endif

    /* Free memory allocated for socket structure.
     */
    os_free(sslsocket, sizeof(osalSSLSocket));
}


/**
****************************************************************************************************

  @brief Accept connection from listening socket.
  @anchor osal_openssl_open

  The osal_openssl_accept() function accepts an incoming connection from listening socket.

  @param   stream Stream pointer representing the listening socket.
  @param   status Pointer to integer into which to store the function status code. Value
           OSAL_SUCCESS (0) indicates that new connection was successfully accepted.
           The value OSAL_NO_NEW_CONNECTION indicates that no new incoming
           connection, was accepted.  All other nonzero values indicate an error,
           See @ref osalStatus "OSAL function return codes" for full list.
           This parameter can be OS_NULL, if no status code is needed.
  @param   flags Flags for creating the socket. Define OSAL_STREAM_DEFAULT for normal operation.
           See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Stream pointer representing the socket, or OS_NULL if the function failed.

****************************************************************************************************
*/
static osalStream osal_openssl_accept(
    osalStream stream,
    os_char *remote_ip_addr,
    os_memsz remote_ip_addr_sz,
    osalStatus *status,
    os_int flags)
{
    osalSSLSocket *sslsocket, *newsslsocket = OS_NULL;
    osalStream newtcpsocket = OS_NULL;
    osalStatus s = OSAL_STATUS_FAILED;

    /* If called with NULL argument, do nothing.
     */
    if (stream == OS_NULL) goto getout;

    sslsocket = (osalSSLSocket*)stream;
    osal_debug_assert(sslsocket->hdr.iface == &osal_tls_iface);

    if (flags == OSAL_STREAM_DEFAULT) {
        flags = sslsocket->open_flags;
    }

    /* Try to accept as normal TCP socket. If no incoming socket to accept, return.
     */
    newtcpsocket = osal_socket_accept(sslsocket->tcpsocket, remote_ip_addr,
        remote_ip_addr_sz, status, flags);
    if (newtcpsocket == OS_NULL)
    {
        /* Status is already set by osal_socket_accept()
         */
        return OS_NULL;
    }

    /* Allocate and clear socket structure.
     */
    newsslsocket = (osalSSLSocket*)os_malloc(sizeof(osalSSLSocket), OS_NULL);
    if (newsslsocket == OS_NULL)
    {
        s = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        goto getout;
    }
    os_memclear(newsslsocket, sizeof(osalSSLSocket));

    /* Save socket stucture pointer, open flags and interface pointer.
     */
    newsslsocket->tcpsocket = newtcpsocket;
    newsslsocket->open_flags = flags;
    newsslsocket->hdr.iface = &osal_tls_iface;

    /* Initialize SSL client and memory BIOs.
     */
    s = osal_openssl_client_init(newsslsocket, OSAL_SSLMODE_SERVER);
    if (s) goto getout;

    /* Success set status code and cast socket structure pointer to stream pointer
       and return it.
     */
    if (status) *status = OSAL_SUCCESS;
    return (osalStream)newsslsocket;

getout:
    /* Opt out on error. If we got far enough to allocate the socket structure.
       Close the event handle (if any) and free memory allocated  for the socket structure.
     */
    if (newsslsocket)
    {
        os_free(newsslsocket, sizeof(osalSSLSocket));
    }

    /* Close socket
     */
    if (newtcpsocket)
    {
        osal_socket_close(newtcpsocket, OSAL_STREAM_DEFAULT);
    }

    /* Set status code and return NULL pointer.
     */
    if (status) *status = s;
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Flush the socket.
  @anchor osal_openssl_flush

  The osal_openssl_flush() function flushes data to be written to stream.

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
static osalStatus osal_openssl_flush(
    osalStream stream,
    os_int flags)
{
    osalSSLSocket *sslsocket;
    osalStatus s;
    os_boolean work_done;

    if (stream)
    {
        sslsocket = (osalSSLSocket*)stream;
        osal_debug_assert(sslsocket->hdr.iface == &osal_tls_iface);

        /* While we have buffered data, encrypt it and move to SSL.
         */
        do
        {
            work_done = OS_FALSE;
            if (sslsocket->encrypt_len > 0)
            {
                s = osal_openssl_do_encrypt(sslsocket);
                switch (s)
                {
                    case OSAL_SUCCESS: work_done = OS_TRUE; break;
                    case OSAL_NOTHING_TO_DO: break;
                    default: return s;
                }
            }
            if (sslsocket->write_len > 0)
            {
                s = osal_openssl_do_sock_write(sslsocket);
                switch (s)
                {
                    case OSAL_SUCCESS: work_done = OS_TRUE; break;
                    case OSAL_NOTHING_TO_DO: break;
                    default: return s;
                }
            }
        }
        while (work_done);

        /* Flush the underlying socket buffers.
         */
        s = osal_socket_flush(sslsocket->tcpsocket, flags);
        return s;
    }
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Write data to socket.
  @anchor osal_openssl_write

  The osal_openssl_write() function writes up to n bytes of data from buffer to socket.

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
static osalStatus osal_openssl_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags)
{
    osalSSLSocket *sslsocket;
    os_memsz n_now;
    osalStatus s;

    *n_written = 0;
    if (stream == OS_NULL) return OSAL_STATUS_FAILED;

    /* Cast stream pointer to socket structure pointer.
     */
    sslsocket = (osalSSLSocket*)stream;
    osal_debug_assert(sslsocket->hdr.iface == &osal_tls_iface);

    /* While we have data left to write...
     */
    while (n > 0)
    {
        /* Limit number of bytes to encrypt now to free bytes in encrypt buffer.
         */
        n_now = n;
        if (n_now + sslsocket->encrypt_len > OSAL_ENCRYPT_BUFFER_SZ)
        {
            n_now = OSAL_ENCRYPT_BUFFER_SZ - sslsocket->encrypt_len;
        }

        /* Store n_now bytes to outgoing buffer to encrypt.
         */
        osal_openssl_send_unencrypted_bytes(sslsocket, (char*)buf, (size_t)n_now);

        /* Update number of bytes left to write, buffer position and total number
           of bytes written. If we are not using blocking mode and we still have
           free space in encryption buffer, do nothing more.
         */
        *n_written += n_now;
        n -= n_now;
        if (sslsocket->encrypt_len < OSAL_ENCRYPT_BUFFER_SZ) break;
        buf += n_now;

        /* Try to encrypt and send some to make space in buffer.
         */
        s = osal_openssl_do_encrypt(sslsocket);
        if (s != OSAL_SUCCESS && s != OSAL_NOTHING_TO_DO) return s;
        s = osal_openssl_do_sock_write(sslsocket);
        if (s != OSAL_SUCCESS && s != OSAL_NOTHING_TO_DO) return s;

        /* If we got nothing encrypted (buffer still full), then just return.
         */
        if (sslsocket->encrypt_len >= OSAL_ENCRYPT_BUFFER_SZ) break;
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Read data from socket.
  @anchor osal_openssl_read

  The osal_openssl_read() function reads up to n bytes of data from socket into buffer.

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
static osalStatus osal_openssl_read(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags)
{
    osalSSLSocket *sslsocket;
    os_char *src;
    os_memsz nprocessed, something_done;
    os_int freespace, bufferedbytes, nstored;
    osalStatus s;
    osalSSLStatus status;

    *n_read = 0;

    if (stream == OS_NULL) return OSAL_STATUS_FAILED;

    /* Cast stream pointer to socket structure pointer, get operating system's socket handle.
     */
    sslsocket = (osalSSLSocket*)stream;
    osal_debug_assert(sslsocket->hdr.iface == &osal_tls_iface);

    do
    {
        something_done = 0;

        /* Read data from socket only if there is space for it in read_buf.
         */
        freespace = OSAL_READ_BUF_SZ - sslsocket->read_buf_n;
        if (freespace > 0)
        {
            s = osal_socket_read(sslsocket->tcpsocket,
                sslsocket->read_buf + sslsocket->read_buf_n,
                freespace, &nprocessed, OSAL_STREAM_DEFAULT);
            if (s) return s;

            sslsocket->read_buf_n += (os_int)nprocessed;
            something_done += nprocessed;
        }

        /* Move data from read buffer to BIO and then decrypt it
         */
        src = sslsocket->read_buf;
        bufferedbytes = sslsocket->read_buf_n;
        while (bufferedbytes > 0)
        {
            nstored = BIO_write(sslsocket->rbio, src, bufferedbytes);
            if (nstored <= 0)
            {
                /* Bio write failure is unrecoverable.
                 */
                if(!BIO_should_retry(sslsocket->rbio))
                {
                    return OSAL_STATUS_FAILED;
                }

                /* Cannot write it all for now. We got still something in read buffer, move
                   it in beginning of the buffer and adjust number of bytes in buffer.
                 */
                if (sslsocket->read_buf != src)
                {
                    os_memmove(sslsocket->read_buf, src, bufferedbytes);
                    sslsocket->read_buf_n = bufferedbytes;
                }
                break;
            }

            /* Advance position in read buffer, less bytes left. Settint sslsocket->read_buf_n
               is needed just to zero it at end.
             */
            src += nstored;
            bufferedbytes -= nstored;
            sslsocket->read_buf_n = bufferedbytes;
            something_done += nstored;
        }

        if (!SSL_is_init_finished(sslsocket->ssl))
        {
            if (osal_openssl_do_ssl_handshake(sslsocket) == OSAL_SSLSTATUS_FAIL)
            {
                return OSAL_STATUS_FAILED;
            }

            if (!SSL_is_init_finished(sslsocket->ssl))
            {
                return OSAL_SUCCESS;
            }
        }

        /* The encrypted data is now in the input bio so now we can perform actual
           read of unencrypted data.
         */
        while (n > 0)
        {
            nprocessed = SSL_read(sslsocket->ssl, buf, (os_int)n);
            if (nprocessed == 0) break;

            /* If not error, advance
             */
            if (nprocessed > 0)
            {
                buf += nprocessed;
                n -= nprocessed;
                *n_read += nprocessed;
            }
            something_done = 1;

            /* Did SSL request to write bytes? This can happen if peer has requested SSL
               renegotiation.
             */
            status = osal_openssl_get_sslstatus(sslsocket->ssl, (os_int)nprocessed);
            if (status == OSAL_SSLSTATUS_WANT_IO)
            {
                do {
                    n = BIO_read(sslsocket->wbio, buf, sizeof(buf));
                    if (n > 0)
                    {
                        osal_openssl_queue_encrypted_bytes(sslsocket, (char*)buf, (size_t)n);
                    }
                    else if (!BIO_should_retry(sslsocket->wbio))
                    {
                        return OSAL_STATUS_FAILED;
                    }
                }
                while (n > 0);
            }

            if (status == OSAL_SSLSTATUS_FAIL)
            {
                return OSAL_STATUS_FAILED;
            }
        }
    }
    while (something_done);

    return OSAL_SUCCESS;
}


#if OSAL_SOCKET_SELECT_SUPPORT
/**
****************************************************************************************************

  @brief Wait for an event from one of sockets.
  @anchor osal_openssl_select

  The osal_openssl_select() function blocks execution of the calling thread until something
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
static osalStatus osal_openssl_select(
    osalStream *streams,
    os_int nstreams,
    osalEvent evnt,
    osalSelectData *selectdata,
    os_int timeout_ms,
    os_int flags)
{
    osalSSLSocket *sslsocket;
    osalStream tcpstreams[OSAL_SOCKET_SELECT_MAX];
    os_int i, ntcpstreams;

    osal_debug_assert(nstreams >= 1 && nstreams <= OSAL_SOCKET_SELECT_MAX);
    ntcpstreams = 0;
    for (i = 0; i < nstreams; i++)
    {
        sslsocket = (osalSSLSocket*)streams[i];
        if (sslsocket == OS_NULL) continue;
        osal_debug_assert(sslsocket->hdr.iface == &osal_tls_iface);
        tcpstreams[ntcpstreams++] = sslsocket->tcpsocket;
    }

    return osal_socket_select(tcpstreams, ntcpstreams, evnt, selectdata, timeout_ms, flags);
}
#endif


/**
****************************************************************************************************

  @brief Initialize the OpenSSL library.
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

    osal_openssl_init(prm);
}


/**
****************************************************************************************************

  @brief Shut down the OpenSSL library.
  @anchor osal_tls_shutdown

  The osal_tls_shutdown() shuts down the underlying OpenSSL library.

  @return  None.

****************************************************************************************************
*/
void osal_tls_shutdown(
    void)
{
    if (osal_global->tls == OS_NULL) return;

    osal_openssl_cleanup();

    os_free(osal_global->tls, sizeof(osalTLS));
    osal_global->tls = OS_NULL;

    osal_socket_shutdown();
}


/**
****************************************************************************************************

  @brief Create and initialize SSL context.
  @anchor osal_openssl_init

  The osal_openssl_init() function sets up a SSL context.

  @return  None.

****************************************************************************************************
*/
static void osal_openssl_init(
    osalSecurityConfig *prm)
{
    SSL_CTX *ctx;
    const os_char *certs_dir;
    os_char path[OSAL_PATH_SZ];

    printf("initialising SSL\n");

    /* SSL library initialisation.
     */
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();

    /* Create the SSL server context.
     */
    ctx = SSL_CTX_new(SSLv23_method());
    osal_global->tls->ctx = ctx;
    if (ctx == 0)
    {
        osal_debug_error("SSL_CTX_new()");
        return;
    }

    /* If no parameters, skip
     */
    if (prm == OS_NULL)
    {
        osal_debug_error("No TLS parameters");
        goto skipit;
    }

    /* If we have no path to directory containing certificates and keys, set testing default.
     */
    certs_dir = prm->certs_dir;
    if (certs_dir == OS_NULL)
    {
        certs_dir = OSAL_FS_ROOT "coderoot/eosal/extensions/tls/keys-and-certs/";
    }

// root_cert_file ?

    /* Load certificate and private key files, and check consistency.
     */
    if (prm->server_cert_file && prm->server_key_file)
    {
        os_strncpy(path, certs_dir, sizeof(path));
        os_strncat(path, prm->server_cert_file, sizeof(path));
        if (SSL_CTX_use_certificate_file(ctx, path,  SSL_FILETYPE_PEM) != 1)
        {
            osal_debug_error("SSL_CTX_use_certificate_file failed");
        }

        os_strncpy(path, certs_dir, sizeof(path));
        os_strncat(path, prm->server_key_file, sizeof(path));
        if (SSL_CTX_use_PrivateKey_file(ctx, path, SSL_FILETYPE_PEM) != 1)
        {
            osal_debug_error("SSL_CTX_use_PrivateKey_file failed");
        }

        //        SSL_CTX_add_extra_chain_cert(3)
 //       SSL_CTX_use_certificate(3).


        /* Make sure the key and certificate file match.
         */
        if (SSL_CTX_check_private_key(ctx) != 1)
        {
            osal_debug_error("SSL_CTX_check_private_key failed");
        }
        /* else
        {
            osal_trace2("certificate and private key loaded and verified\n");
        } */
    }

    /* Client verify server certificate
     */
    osal_openssl_client_setup(prm, certs_dir);

    /* if (prm->client_cert_chain_file)
    {
        os_strncpy(path, certs_dir, sizeof(path));
        os_strncat(path, prm->client_cert_chain_file, sizeof(path));
        if ( SSL_CTX_load_verify_locations(ctx, path, NULL) != 1 )
        {
            osal_debug_error("SSL_CTX_load_verify_locations failed");
        }

        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, osal_openssl_verify_callback);
        SSL_CTX_set_verify_depth(ctx, 1);
    } */

skipit:
    /* Recommended to avoid SSLv2 & SSLv3.
     */
    SSL_CTX_set_options(ctx, SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);
}


static void osal_openssl_client_setup(
    osalSecurityConfig *prm,
    const os_char *certs_dir)
{
    osalTLS *t;
    os_char path[OSAL_PATH_SZ];
    const os_char *file_name;
    osPersistentBlockNr block_nr;
    osalStatus s;
    os_char *block;
    os_memsz block_sz;

    t = osal_global->tls;

    file_name = prm->client_cert_chain_file;

file_name = "myhome-bundle.crt";

    if (file_name) if (!osal_char_isdigit(*file_name) && *file_name != '\0')
    {
        os_strncpy(path, certs_dir, sizeof(path));
        os_strncat(path, file_name, sizeof(path));
        if ( SSL_CTX_load_verify_locations(t->ctx, path, NULL) != 1 )
        {
            osal_debug_error("SSL_CTX_load_verify_locations failed");

           /* Mark to network info that we need certificate chain.
            */
           osal_set_network_state_int(OSAL_NS_NO_CERT_CHAIN, 0, OS_TRUE);
        }

        SSL_CTX_set_verify(t->ctx, SSL_VERIFY_PEER, osal_openssl_verify_callback);
        SSL_CTX_set_verify_depth(t->ctx, 1);

//        SSL_CTX_use_certificate(ctx, X509 *x).
//        SSL_CTX_add_extra_chain_cert(3)

        return;
    }

    block_nr = (osPersistentBlockNr)osal_str_to_int(file_name, OS_NULL);
    if (block_nr == 0) block_nr = OS_PBNR_CLIENT_CERT_CHAIN;
    s = os_load_persistent_malloc(block_nr, &block, &block_sz);
    if (s != OSAL_SUCCESS && s != OSAL_MEMORY_ALLOCATED)
    {
        osal_debug_error_int("os_load_persistent_malloc failed ", block_nr);

       /* Mark to network info that we need certificate chain.
        */
       osal_set_network_state_int(OSAL_NS_NO_CERT_CHAIN, 0, OS_TRUE);
    }

    if (s == OSAL_MEMORY_ALLOCATED)
    {
        os_free(block, block_sz);
    }

/*
C++/OpenSSL: Use root CA from buffer rather than file (SSL_CTX_load_verify_locations)


The function SSL_CTX_get_cert_store() can be used to get a handle to the certificate store
used for verification (X509_STORE *), and the X509_STORE_add_cert() function
(in openssl/x509_vfy.h) can then be used to add a certificate directly to that certificate store.
*/

}

#if 0

static CURLcode sslctx_function(CURL *curl, void *sslctx, void *parm)
{
  X509 *cert = NULL;
  BIO *bio = NULL;
  BIO *kbio = NULL;
  RSA *rsa = NULL;
  int ret;

  const char *mypem = /* www.cacert.org */
    "-----BEGIN CERTIFICATE-----\n"\
    "MIIHPTCCBSWgAwIBAgIBADANBgkqhkiG9w0BAQQFADB5MRAwDgYDVQQKEwdSb290\n"\
    "IENBMR4wHAYDVQQLExVodHRwOi8vd3d3LmNhY2VydC5vcmcxIjAgBgNVBAMTGUNB\n"\
    "IENlcnQgU2lnbmluZyBBdXRob3JpdHkxITAfBgkqhkiG9w0BCQEWEnN1cHBvcnRA\n"\
    "Y2FjZXJ0Lm9yZzAeFw0wMzAzMzAxMjI5NDlaFw0zMzAzMjkxMjI5NDlaMHkxEDAO\n"\
    "BgNVBAoTB1Jvb3QgQ0ExHjAcBgNVBAsTFWh0dHA6Ly93d3cuY2FjZXJ0Lm9yZzEi\n"\
    "MCAGA1UEAxMZQ0EgQ2VydCBTaWduaW5nIEF1dGhvcml0eTEhMB8GCSqGSIb3DQEJ\n"\
    "ARYSc3VwcG9ydEBjYWNlcnQub3JnMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIIC\n"\
    "CgKCAgEAziLA4kZ97DYoB1CW8qAzQIxL8TtmPzHlawI229Z89vGIj053NgVBlfkJ\n"\
    "8BLPRoZzYLdufujAWGSuzbCtRRcMY/pnCujW0r8+55jE8Ez64AO7NV1sId6eINm6\n"\
    "zWYyN3L69wj1x81YyY7nDl7qPv4coRQKFWyGhFtkZip6qUtTefWIonvuLwphK42y\n"\
    "fk1WpRPs6tqSnqxEQR5YYGUFZvjARL3LlPdCfgv3ZWiYUQXw8wWRBB0bF4LsyFe7\n"\
    "w2t6iPGwcswlWyCR7BYCEo8y6RcYSNDHBS4CMEK4JZwFaz+qOqfrU0j36NK2B5jc\n"\
    "G8Y0f3/JHIJ6BVgrCFvzOKKrF11myZjXnhCLotLddJr3cQxyYN/Nb5gznZY0dj4k\n"\
    "epKwDpUeb+agRThHqtdB7Uq3EvbXG4OKDy7YCbZZ16oE/9KTfWgu3YtLq1i6L43q\n"\
    "laegw1SJpfvbi1EinbLDvhG+LJGGi5Z4rSDTii8aP8bQUWWHIbEZAWV/RRyH9XzQ\n"\
    "QUxPKZgh/TMfdQwEUfoZd9vUFBzugcMd9Zi3aQaRIt0AUMyBMawSB3s42mhb5ivU\n"\
    "fslfrejrckzzAeVLIL+aplfKkQABi6F1ITe1Yw1nPkZPcCBnzsXWWdsC4PDSy826\n"\
    "YreQQejdIOQpvGQpQsgi3Hia/0PsmBsJUUtaWsJx8cTLc6nloQsCAwEAAaOCAc4w\n"\
    "ggHKMB0GA1UdDgQWBBQWtTIb1Mfz4OaO873SsDrusjkY0TCBowYDVR0jBIGbMIGY\n"\
    "gBQWtTIb1Mfz4OaO873SsDrusjkY0aF9pHsweTEQMA4GA1UEChMHUm9vdCBDQTEe\n"\
    "MBwGA1UECxMVaHR0cDovL3d3dy5jYWNlcnQub3JnMSIwIAYDVQQDExlDQSBDZXJ0\n"\
    "IFNpZ25pbmcgQXV0aG9yaXR5MSEwHwYJKoZIhvcNAQkBFhJzdXBwb3J0QGNhY2Vy\n"\
    "dC5vcmeCAQAwDwYDVR0TAQH/BAUwAwEB/zAyBgNVHR8EKzApMCegJaAjhiFodHRw\n"\
    "czovL3d3dy5jYWNlcnQub3JnL3Jldm9rZS5jcmwwMAYJYIZIAYb4QgEEBCMWIWh0\n"\
    "dHBzOi8vd3d3LmNhY2VydC5vcmcvcmV2b2tlLmNybDA0BglghkgBhvhCAQgEJxYl\n"\
    "aHR0cDovL3d3dy5jYWNlcnQub3JnL2luZGV4LnBocD9pZD0xMDBWBglghkgBhvhC\n"\
    "AQ0ESRZHVG8gZ2V0IHlvdXIgb3duIGNlcnRpZmljYXRlIGZvciBGUkVFIGhlYWQg\n"\
    "b3ZlciB0byBodHRwOi8vd3d3LmNhY2VydC5vcmcwDQYJKoZIhvcNAQEEBQADggIB\n"\
    "ACjH7pyCArpcgBLKNQodgW+JapnM8mgPf6fhjViVPr3yBsOQWqy1YPaZQwGjiHCc\n"\
    "nWKdpIevZ1gNMDY75q1I08t0AoZxPuIrA2jxNGJARjtT6ij0rPtmlVOKTV39O9lg\n"\
    "18p5aTuxZZKmxoGCXJzN600BiqXfEVWqFcofN8CCmHBh22p8lqOOLlQ+TyGpkO/c\n"\
    "gr/c6EWtTZBzCDyUZbAEmXZ/4rzCahWqlwQ3JNgelE5tDlG+1sSPypZt90Pf6DBl\n"\
    "Jzt7u0NDY8RD97LsaMzhGY4i+5jhe1o+ATc7iwiwovOVThrLm82asduycPAtStvY\n"\
    "sONvRUgzEv/+PDIqVPfE94rwiCPCR/5kenHA0R6mY7AHfqQv0wGP3J8rtsYIqQ+T\n"\
    "SCX8Ev2fQtzzxD72V7DX3WnRBnc0CkvSyqD/HMaMyRa+xMwyN2hzXwj7UfdJUzYF\n"\
    "CpUCTPJ5GhD22Dp1nPMd8aINcGeGG7MW9S/lpOt5hvk9C8JzC6WZrG/8Z7jlLwum\n"\
    "GCSNe9FINSkYQKyTYOGWhlC0elnYjyELn8+CkcY7v2vcB5G5l1YjqrZslMZIBjzk\n"\
    "zk6q5PYvCdxTby78dOs6Y5nCpqyJvKeyRKANihDjbPIky/qbn3BHLt4Ui9SyIAmW\n"\
    "omTxJBzcoTWcFbLUvFUufQb1nA5V9FrWk9p2rSVzTMVD\n"\
    "-----END CERTIFICATE-----\n";

/*replace the XXX with the actual RSA key*/
  const char *mykey =
    "-----BEGIN RSA PRIVATE KEY-----\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n"\
    "-----END RSA PRIVATE KEY-----\n";

  (void)curl; /* avoid warnings */
  (void)parm; /* avoid warnings */

  /* get a BIO */
  bio = BIO_new_mem_buf((char *)mypem, -1);

  if(bio == NULL) {
    printf("BIO_new_mem_buf failed\n");
  }

  /* use it to read the PEM formatted certificate from memory into an X509
   * structure that SSL can use
   */
  cert = PEM_read_bio_X509(bio, NULL, 0, NULL);
  if(cert == NULL) {
    printf("PEM_read_bio_X509 failed...\n");
  }

  /*tell SSL to use the X509 certificate*/
  ret = SSL_CTX_use_certificate((SSL_CTX*)sslctx, cert);
  if(ret != 1) {
    printf("Use certificate failed\n");
  }

  /*create a bio for the RSA key*/
  kbio = BIO_new_mem_buf((char *)mykey, -1);
  if(kbio == NULL) {
    printf("BIO_new_mem_buf failed\n");
  }

  /*read the key bio into an RSA object*/
  rsa = PEM_read_bio_RSAPrivateKey(kbio, NULL, 0, NULL);
  if(rsa == NULL) {
    printf("Failed to create key bio\n");
  }

  /*tell SSL to use the RSA key from memory*/
  ret = SSL_CTX_use_RSAPrivateKey((SSL_CTX*)sslctx, rsa);
  if(ret != 1) {
    printf("Use Key failed\n");
  }

  /* free resources that have been allocated by openssl functions */
  if(bio)
    BIO_free(bio);

  if(kbio)
    BIO_free(kbio);

  if(rsa)
    RSA_free(rsa);

  if(cert)
    X509_free(cert);

  /* all set to go */
  return CURLE_OK;
}
#endif



/**
****************************************************************************************************

  @brief Clean up OpenSSL library.
  @anchor osal_openssl_cleanup

  The osal_openssl_cleanup() function...

  @param   sslsocket Stream pointer representing the SSL socket.
  @return  OSAL_SUCCESS if all fine. Other return values indicate an error.

****************************************************************************************************
*/
static void osal_openssl_cleanup(void)
{
    ERR_free_strings();
    EVP_cleanup();
}


/**
****************************************************************************************************

  @brief Initialize SSL client and memory BIOs.
  @anchor osal_openssl_client_init

  The osal_openssl_client_init() function...

  @param   sslsocket Stream pointer representing the SSL socket.
  @return  OSAL_SUCCESS if all fine. Other return values indicate an error.

****************************************************************************************************
*/
static osalStatus osal_openssl_client_init(
    osalSSLSocket *sslsocket,
    osalSSLMode mode)
{
    osalStatus s = OSAL_SUCCESS;

    sslsocket->rbio = BIO_new(BIO_s_mem());
    sslsocket->wbio = BIO_new(BIO_s_mem());
    sslsocket->ssl = SSL_new(osal_global->tls->ctx);

    if (mode == OSAL_SSLMODE_SERVER)
    {
        SSL_set_accept_state(sslsocket->ssl);  /* ssl server mode */
    }
    else if (mode == OSAL_SSLMODE_CLIENT)
    {
        SSL_set_connect_state(sslsocket->ssl); /* ssl client mode */
    }

    SSL_set_bio(sslsocket->ssl, sslsocket->rbio, sslsocket->wbio);

    return s;
}


/**
****************************************************************************************************

  @brief Clean up the SSL client memory BIO.
  @anchor osal_openssl_client_cleanup

  The osal_openssl_client_cleanup() function...

  @param   sslsocket Stream pointer representing the SSL socket.
  @return  None.

****************************************************************************************************
*/
static void osal_openssl_client_cleanup(
    osalSSLSocket *sslsocket)
{
    if (sslsocket->ssl)
    {
        SSL_shutdown(sslsocket->ssl);
        SSL_free(sslsocket->ssl);   /* free the SSL object and its BIO's */
        free(sslsocket->write_buf);
        free(sslsocket->encrypt_buf);
    }
}


/**
****************************************************************************************************

  @brief Get return code of a SLL operation.
  @anchor osal_openssl_get_sslstatus

  The osal_openssl_get_sslstatus() function obtains the return value of a SSL operation and
  converts into a simplified code, which is easier to examine for failure.

  @return  Simplified SSL return code, one of OSAL_SSLSTATUS_OK, OSAL_SSLSTATUS_WANT_IO or
           OSAL_SSLSTATUS_FAIL.

****************************************************************************************************
*/
static osalSSLStatus osal_openssl_get_sslstatus(
    SSL* ssl,
    os_int n)
{
    switch (SSL_get_error(ssl, n))
    {
        case SSL_ERROR_NONE:
            return OSAL_SSLSTATUS_OK;

        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_READ:
            return OSAL_SSLSTATUS_WANT_IO;

        case SSL_ERROR_ZERO_RETURN:
        case SSL_ERROR_SYSCALL:
        default:
            return OSAL_SSLSTATUS_FAIL;
    }
}


/**
****************************************************************************************************

  @brief Send unencrypted data.
  @anchor osal_openssl_send_unencrypted_bytes

  The osal_openssl_send_unencrypted_bytes() function handles request to send unencrypted data
  to the SSL.  All we do here is just queue the data into the encrypt_buf for later processing
  by the SSL object.

  @param   sslsocket Stream pointer representing the SSL socket.
  @param   buf Pointer to data to send.
  @param   len Number of bytes in buffer.
  @return  None.

****************************************************************************************************
*/
static void osal_openssl_send_unencrypted_bytes(
    osalSSLSocket *sslsocket,
    const char *buf,
    size_t len)
{
    sslsocket->encrypt_buf = (char*)realloc(sslsocket->encrypt_buf, sslsocket->encrypt_len + len);
    memcpy(sslsocket->encrypt_buf + sslsocket->encrypt_len, buf, len);
    sslsocket->encrypt_len += len;
}


/**
****************************************************************************************************

  @brief Queue encrypted bytes.
  @anchor osal_openssl_queue_encrypted_bytes

  The osal_openssl_queue_encrypted_bytes() function queues encrypted bytes. Should only be used
  when the SSL object has requested a write operation.

  @param   sslsocket Stream pointer representing the SSL socket.
  @param   buf Pointer to data to send.
  @param   len Number of bytes in buffer.
  @return  None.

****************************************************************************************************
*/
static void osal_openssl_queue_encrypted_bytes(
    osalSSLSocket *sslsocket,
    const char *buf,
    size_t len)
{
    sslsocket->write_buf = (char*)realloc(sslsocket->write_buf, sslsocket->write_len + len);
    memcpy(sslsocket->write_buf + sslsocket->write_len, buf, len);
    sslsocket->write_len += len;
}


/**
****************************************************************************************************

  @brief SSL handshake.
  @anchor osal_openssl_do_ssl_handshake

  The osal_openssl_do_ssl_handshake() function...

  @param   sslsocket Stream pointer representing the SSL socket.
  @return  SSL status.

****************************************************************************************************
*/
static osalSSLStatus osal_openssl_do_ssl_handshake(
    osalSSLSocket *sslsocket)
{
    char buf[OSAL_SSL_DEFAULT_BUF_SIZE];
    osalSSLStatus status;

    os_int n = SSL_do_handshake(sslsocket->ssl);
    status = osal_openssl_get_sslstatus(sslsocket->ssl, n);

    /* Did SSL request to write bytes?
     */
    if (status == OSAL_SSLSTATUS_WANT_IO)
    {
        do {
            n = BIO_read(sslsocket->wbio, buf, sizeof(buf));
            if (n > 0)
            {
                osal_openssl_queue_encrypted_bytes(sslsocket, buf, n);
            }
            else if (!BIO_should_retry(sslsocket->wbio))
            {
                return OSAL_SSLSTATUS_FAIL;
            }
        }
        while (n > 0);
    }

    return status;
}


/**
****************************************************************************************************

  @brief Encrypt data to SLL socket.
  @anchor osal_openssl_do_encrypt

  The osal_openssl_do_encrypt() function processes outbound unencrypted data that is waiting
  to be encrypted. The waiting data resides in encrypt_buf. It needs to be passed into the SSL
  object for encryption, which in turn generates the encrypted bytes that then
  will be queued for later socket write.

  @param   sslsocket Stream pointer representing the SSL socket.
  @return  The function returns OSAL_SUCCESS if some data was succesfully encrypted.
           Return value OSAL_NOTHING_TO_DO indicates that there is nothing to
           encrypt. Other return values indicate an error.

****************************************************************************************************
*/
static osalStatus osal_openssl_do_encrypt(
    osalSSLSocket *sslsocket)
{
    char buf[OSAL_SSL_DEFAULT_BUF_SIZE];
    osalSSLStatus status;
    osalStatus s;
    os_int n;

    if (!SSL_is_init_finished(sslsocket->ssl))
        return OSAL_NOTHING_TO_DO;

    s = OSAL_NOTHING_TO_DO;
    while (sslsocket->encrypt_len > 0)
    {
        n = SSL_write(sslsocket->ssl, sslsocket->encrypt_buf, (int)sslsocket->encrypt_len);
        status = osal_openssl_get_sslstatus(sslsocket->ssl, n);

        if (n > 0)
        {
            s = OSAL_SUCCESS;

            /* Consume the waiting bytes that have been used by SSL.
             */
            if ((size_t) n < sslsocket->encrypt_len)
            {
                memmove(sslsocket->encrypt_buf, sslsocket->encrypt_buf + n, sslsocket->encrypt_len - n);
            }
            sslsocket->encrypt_len -= n;
            sslsocket->encrypt_buf = (char*)realloc(sslsocket->encrypt_buf, sslsocket->encrypt_len);

            /* Take the output of the SSL object and queue it for socket write.
             */
            do
            {
                n = BIO_read(sslsocket->wbio, buf, sizeof(buf));
                if (n > 0)
                {
                    osal_openssl_queue_encrypted_bytes(sslsocket, buf, n);
                }
                else if (!BIO_should_retry(sslsocket->wbio))
                {
                    return OSAL_STATUS_FAILED;
                }
            }
            while (n > 0);
        }

        if (status == OSAL_SSLSTATUS_FAIL)
            return OSAL_STATUS_FAILED;

        if (n==0)
            break;
    }
    return s;
}


/**
****************************************************************************************************

  @brief Write encrypted bytes to the socket.
  @anchor osal_openssl_do_sock_write

  The osal_openssl_do_sock_write() function...

  @param   sslsocket Stream pointer representing the SSL socket.
  @return  The function returns OSAL_SUCCESS if some data was succesfully encrypted.
           Return value OSAL_NOTHING_TO_DO indicates that there is nothing to
           encrypt. Other return values indicate an error.

****************************************************************************************************
*/
static osalStatus osal_openssl_do_sock_write(
    osalSSLSocket *sslsocket)
{
    os_memsz n;
    osalStatus s;

    s = osal_socket_write(sslsocket->tcpsocket, sslsocket->write_buf,
        sslsocket->write_len, &n, OSAL_STREAM_DEFAULT);

    if (n > 0)
    {
        if ((size_t)n < sslsocket->write_len)
        {
            memmove(sslsocket->write_buf, sslsocket->write_buf + n, sslsocket->write_len - (size_t)n);
        }

        sslsocket->write_len -= (size_t)n;
        sslsocket->write_buf = (char*)realloc(sslsocket->write_buf, sslsocket->write_len);
        return OSAL_SUCCESS;
    }

    return (s == OSAL_SUCCESS && n == 0) ? OSAL_NOTHING_TO_DO : s;
}


static int osal_openssl_verify_callback(
    int preverify,
    X509_STORE_CTX* x509_ctx)
{
    /* For error codes, see http://www.openssl.org/docs/apps/verify.html  */
    int err = X509_STORE_CTX_get_error(x509_ctx);

    if (preverify == 0)
    {
        if(err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY)
        {
            osal_debug_error("Error = X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY");
        }
        else if(err == X509_V_ERR_CERT_UNTRUSTED)
        {
            osal_debug_error("Error = X509_V_ERR_CERT_UNTRUSTED");
        }
        else if(err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN)
        {
            osal_debug_error("Error = X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN");
        }
        else if(err == X509_V_ERR_CERT_NOT_YET_VALID)
        {
            osal_trace2("Remark = X509_V_ERR_CERT_NOT_YET_VALID (ignored)");
        }
        else if(err == X509_V_ERR_CERT_HAS_EXPIRED)
        {
            osal_trace2("Remark = X509_V_ERR_CERT_HAS_EXPIRED (ignored)");
        }
        else if(err == X509_V_OK)
        {
            osal_trace2("X509_V_OK");
        }
        else
        {
            osal_debug_error_int("Error = ", err);
        }
    }

#if OSAL_RELAX_SECURITY
    return 1;
#else
    if (err == X509_V_OK ||
        err == X509_V_ERR_CERT_HAS_EXPIRED ||
        err == X509_V_ERR_CERT_NOT_YET_VALID)
        return 1;

    return preverify;
#endif
}

/** Stream interface for OSAL sockets. This is structure osalStreamInterface filled with
    function pointers to OSAL sockets implementation.
 */
OS_FLASH_MEM osalStreamInterface osal_tls_iface
 = {OSAL_STREAM_IFLAG_SECURE,
    osal_openssl_open,
    osal_openssl_close,
    osal_openssl_accept,
    osal_openssl_flush,
    osal_stream_default_seek,
    osal_openssl_write,
    osal_openssl_read,
    osal_stream_default_write_value,
    osal_stream_default_read_value,
    osal_stream_default_get_parameter, /* This does not access parameters of contained TCP socket? */
    osal_stream_default_set_parameter,
#if OSAL_SOCKET_SELECT_SUPPORT
    osal_openssl_select};
#else
    osal_stream_default_select};
#endif

#endif
