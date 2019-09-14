/**

  @file    tls/common/osal_openssl.c
  @brief   OSAL sockets API OpenSSL implementation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    20.8.2019

  OpenSSL for sockets.

  Original copyright notice and credits:
  This is based on example work by Mr Darren Smith. Original Copyright (c) 2017 Darren Smith.
  The example is free software; you can redistribute it and/or modify it under the terms of
  the MIT license. See LICENSE for details.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_OPENSSL_SUPPORT

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

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

/* Global SSL context.
 */
SSL_CTX *ctx;

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
    os_uchar read_buf[OSAL_READ_BUF_SZ];
    os_int read_buf_n;
}
osalSSLSocket;


/** Socket library initialized flag.
 */
os_boolean osal_tls_initialized = OS_FALSE;


/* Prototypes for forward referred static functions.
 */
static void osal_openssl_init(
    const os_char *certfile,
    const os_char *keyfile);

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
          - OSAL_STREAM_BLOCKING: Open socket in blocking mode.

		  See @ref osalStreamFlags "Flags for Stream Functions" for full list of stream flags.

  @return Stream pointer representing the socket, or OS_NULL if the function failed.

****************************************************************************************************
*/
osalStream osal_openssl_open(
    const os_char *parameters,
	void *option,
	osalStatus *status,
	os_int flags)
{
    osalStream tcpsocket = OS_NULL;
    osalSSLSocket *sslsocket = OS_NULL;
    osalStatus s;

    /* Initialize TLS sockets library, if not already initialized. Here we have no certificate
       or key, so this may well not work (at least for a server).
     */
    if (!osal_tls_initialized)
    {
        osal_tls_initialize(OS_NULL);
    }

    /* Connect or listen socket.
     */
    tcpsocket = osal_socket_open(parameters, option, status, flags);
    if (tcpsocket == OS_NULL) return OS_NULL;

    /* Allocate and clear socket structure.
	 */
    sslsocket = (osalSSLSocket*)os_malloc(sizeof(osalSSLSocket), OS_NULL);
    if (sslsocket == OS_NULL)
	{
        s = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
		goto getout;
	}
    os_memclear(sslsocket, sizeof(osalSSLSocket));

    /* Save socket stucture pointer, open flags and interface pointer.
	 */
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
        osal_socket_close(tcpsocket);
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
void osal_openssl_close(
	osalStream stream)
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
    osal_socket_close(sslsocket->tcpsocket);

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
		   The value OSAL_STATUS_NO_NEW_CONNECTION indicates that no new incoming 
		   connection, was accepted.  All other nonzero values indicate an error,
           See @ref osalStatus "OSAL function return codes" for full list.
		   This parameter can be OS_NULL, if no status code is needed. 
  @param   flags Flags for creating the socket. Define OSAL_STREAM_DEFAULT for normal operation.
		   See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.
  @return  Stream pointer representing the socket, or OS_NULL if the function failed.

****************************************************************************************************
*/
osalStream osal_openssl_accept(
	osalStream stream,
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

    /* Try to accept as normal TCP socket. If no incoming socket to accept, return.
     */
    newtcpsocket = osal_socket_accept(sslsocket->tcpsocket, status, flags);
    if (newtcpsocket == OS_NULL) return OS_NULL;

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
        osal_socket_close(newtcpsocket);
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
osalStatus osal_openssl_flush(
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
                    case OSAL_STATUS_NOTHING_TO_DO: break;
                    default: return s;
                }
            }
            if (sslsocket->write_len > 0)
            {
                s = osal_openssl_do_sock_write(sslsocket);
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
osalStatus osal_openssl_write(
	osalStream stream,
	const os_uchar *buf,
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
        if ((flags & OSAL_STREAM_BLOCKING) == 0 &&
            sslsocket->encrypt_len < OSAL_ENCRYPT_BUFFER_SZ) break;
        buf += n_now;

        /* Try to encrypt and send some to make space in buffer.
         */
        s = osal_openssl_do_encrypt(sslsocket);
        if (s != OSAL_SUCCESS && s != OSAL_STATUS_NOTHING_TO_DO) return s;
        s = osal_openssl_do_sock_write(sslsocket);
        if (s != OSAL_SUCCESS && s != OSAL_STATUS_NOTHING_TO_DO) return s;

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
		   The OSAL_STREAM_PEEK flag causes the function to return data in socket, but nothing
		   will be removed from the socket.
		   See @ref osalStreamFlags "Flags for Stream Functions" for full list of flags.

  @return  Function status code. Value OSAL_SUCCESS (0) indicates success and all nonzero values
		   indicate an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_openssl_read(
	osalStream stream,
	os_uchar *buf,
	os_memsz n,
	os_memsz *n_read,
	os_int flags)
{
    osalSSLSocket *sslsocket;
    os_uchar *src;
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
osalStatus osal_openssl_select(
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

  @return  None.

****************************************************************************************************
*/
void osal_tls_initialize(
    osalTLSParam *prm)
{
    const os_char
        *certfile = OS_NULL,
        *keyfile = OS_NULL;

    osal_socket_initialize();

    if (prm)
    {
        certfile = prm->certfile;
        keyfile = prm->keyfile;
    }

    osal_openssl_init(certfile, keyfile);

    /* Set socket library initialized flag.
     */
    osal_tls_initialized = OS_TRUE;
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
    if (osal_tls_initialized)
    {
        osal_openssl_cleanup();
        osal_tls_initialized = OS_FALSE;
        osal_socket_shutdown();
    }
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
    const os_char *certfile,
    const os_char *keyfile)
{
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
    if (ctx == 0)
    {
        osal_debug_error("SSL_CTX_new()");
        return;
    }

    /* Load certificate and private key files, and check consistency.
     */
    if (certfile && keyfile)
    {
        if (SSL_CTX_use_certificate_file(ctx, certfile,  SSL_FILETYPE_PEM) != 1)
        {
            osal_debug_error("SSL_CTX_use_certificate_file failed");
        }

        if (SSL_CTX_use_PrivateKey_file(ctx, keyfile, SSL_FILETYPE_PEM) != 1)
        {
            osal_debug_error("SSL_CTX_use_PrivateKey_file failed");
        }

        /* Make sure the key and certificate file match.
         */
        if (SSL_CTX_check_private_key(ctx) != 1)
        {
            osal_debug_error("SSL_CTX_check_private_key failed");
        }
        else
        {
            osal_trace("certificate and private key loaded and verified\n");
        }
    }

    /* Recommended to avoid SSLv2 & SSLv3.
     */
    SSL_CTX_set_options(ctx, SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);
}

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
//    int fd,
    osalSSLMode mode)
{
    osalStatus s = OSAL_SUCCESS;

    // sslsocket->fd = fd;

    sslsocket->rbio = BIO_new(BIO_s_mem());
    sslsocket->wbio = BIO_new(BIO_s_mem());
    sslsocket->ssl = SSL_new(ctx);

    if (mode == OSAL_SSLMODE_SERVER)
    {
        SSL_set_accept_state(sslsocket->ssl);  /* ssl server mode */
  //      s = OSAL_STATUS_FAILED;
    }
    else if (mode == OSAL_SSLMODE_CLIENT)
    {
        SSL_set_connect_state(sslsocket->ssl); /* ssl client mode */
//        s = OSAL_STATUS_FAILED;
    }

    SSL_set_bio(sslsocket->ssl, sslsocket->rbio, sslsocket->wbio);

//    sslsocket->io_on_read = print_unencrypted_data; ???????????????????????????????????????????????????????????????????????
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

    int n = SSL_do_handshake(sslsocket->ssl);
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
           Return value OSAL_STATUS_NOTHING_TO_DO indicates that there is nothing to
           encrypt. Other return values indicate an error.

****************************************************************************************************
*/
static osalStatus osal_openssl_do_encrypt(
    osalSSLSocket *sslsocket)
{
    char buf[OSAL_SSL_DEFAULT_BUF_SIZE];
    osalSSLStatus status;
    osalStatus s;
    int n;

    if (!SSL_is_init_finished(sslsocket->ssl))
        return OSAL_STATUS_NOTHING_TO_DO;

    s = OSAL_STATUS_NOTHING_TO_DO;
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
           Return value OSAL_STATUS_NOTHING_TO_DO indicates that there is nothing to
           encrypt. Other return values indicate an error.

****************************************************************************************************
*/
static osalStatus osal_openssl_do_sock_write(
    osalSSLSocket *sslsocket)
{
    os_memsz n;
    osalStatus s;

    s = osal_socket_write(sslsocket->tcpsocket, (os_uchar*)sslsocket->write_buf,
        sslsocket->write_len, &n, OSAL_STREAM_DEFAULT);

    if (n > 0)
    {
        if ((size_t)n < sslsocket->write_len)
        {
            memmove(sslsocket->write_buf, sslsocket->write_buf+n, sslsocket->write_len - (size_t)n);
        }

        sslsocket->write_len -= (size_t)n;
        sslsocket->write_buf = (char*)realloc(sslsocket->write_buf, sslsocket->write_len);
        return OSAL_SUCCESS;
    }

    return (s == OSAL_SUCCESS && n == 0) ? OSAL_STATUS_NOTHING_TO_DO : s;
}


/** Stream interface for OSAL sockets. This is structure osalStreamInterface filled with
    function pointers to OSAL sockets implementation.
 */
osalStreamInterface osal_tls_iface
 = {osal_openssl_open,
    osal_openssl_close,
    osal_openssl_accept,
    osal_openssl_flush,
	osal_stream_default_seek,
    osal_openssl_write,
    osal_openssl_read,
	osal_stream_default_write_value,
	osal_stream_default_read_value,
    osal_stream_default_get_parameter,
    osal_stream_default_set_parameter,
#if OSAL_SOCKET_SELECT_SUPPORT
    osal_openssl_select};
#else
    osal_stream_default_select};
#endif

#endif
