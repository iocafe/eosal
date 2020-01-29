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

// #include <string.h>

/* Random number generator context.
 */
static mbedtls_ctr_drbg_context ctr_drbg;
static mbedtls_entropy_context entropy;


/** This enum contols whether the SSL connection needs to initiate the SSL handshake.
 */
typedef enum osalSSLMode
{
    OSAL_SSLMODE_SERVER,
    OSAL_SSLMODE_CLIENT
}
osalSSLMode;


/* Global SSL context.
 */

// SSL_CTX *ctx;

#define OSAL_SSL_DEFAULT_BUF_SIZE 512

#define OSAL_ENCRYPT_BUFFER_SZ 256


#define OSAL_READ_BUF_SZ 512


/** Mbed TLS specific socket data structure. OSAL functions cast their own stream structure
    pointers to osalStream pointers.
 */
typedef struct osalSSLSocket
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

    mbedtls_net_context fd;

}
osalSSLSocket;


/** Socket library initialized flag.
 */
os_boolean osal_tls_initialized = OS_FALSE;


/* Prototypes for forward referred static functions.
 */
static void osal_mbedtls_init(
    osalSecurityConfig *prm);

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
          - OSAL_STREAM_BLOCKING: Open socket in blocking mode.

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
    osalSSLSocket *sslsocket = OS_NULL;
    osalStatus s;
    os_char host[OSAL_HOST_BUF_SZ];

    /* If not initialized.
     */
    if (!osal_tls_initialized)
    {
        if (status) *status = OSAL_STATUS_FAILED;
        return OS_NULL;
    }

parameters = "stalin:6367";

    /* Connect or listen socket. Make sure to use TLS default port if unspecified.
     */
    osal_socket_embed_default_port(parameters,
        host, sizeof(host), IOC_DEFAULT_TLS_PORT);
    tcpsocket = osal_socket_open(host, option, status, flags);
    if (tcpsocket == OS_NULL) return OS_NULL;

#if 0
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
     *
     */
    if ((flags & (OSAL_STREAM_LISTEN|OSAL_STREAM_CONNECT)) == OSAL_STREAM_CONNECT)
	{
        /* Initialize SSL client and memory BIOs.
        */
        s = osal_mbedtls_client_init(sslsocket, OSAL_SSLMODE_CLIENT);
        if (s) goto getout;

        if (osal_mbedtls_do_ssl_handshake(sslsocket) == OSAL_SSLSTATUS_FAIL)
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
#endif
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
    osalSSLSocket *sslsocket;

	/* If called with NULL argument, do nothing.
	 */
	if (stream == OS_NULL) return;

    /* Cast stream pointer to socket structure pointer, get operating system's socket handle.
	 */
    sslsocket = (osalSSLSocket*)stream;
    osal_debug_assert(sslsocket->hdr.iface == &osal_tls_iface);


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
#if 0
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
    newtcpsocket = osal_socket_accept(sslsocket->tcpsocket, remote_ip_addr,
        remote_ip_addr_sz, status, flags);
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
    s = osal_mbedtls_client_init(newsslsocket, OSAL_SSLMODE_SERVER);
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
#endif
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
#if 0
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
                s = osal_mbedtls_do_encrypt(sslsocket);
                switch (s)
                {
                    case OSAL_SUCCESS: work_done = OS_TRUE; break;
                    case OSAL_STATUS_NOTHING_TO_DO: break;
                    default: return s;
                }
            }
            if (sslsocket->write_len > 0)
            {
                s = osal_mbedtls_do_sock_write(sslsocket);
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
#if 0
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
        osal_mbedtls_send_unencrypted_bytes(sslsocket, (char*)buf, (size_t)n_now);

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
        s = osal_mbedtls_do_encrypt(sslsocket);
        if (s != OSAL_SUCCESS && s != OSAL_STATUS_NOTHING_TO_DO) return s;
        s = osal_mbedtls_do_sock_write(sslsocket);
        if (s != OSAL_SUCCESS && s != OSAL_STATUS_NOTHING_TO_DO) return s;

        /* If we got nothing encrypted (buffer still full), then just return.
         */
        if (sslsocket->encrypt_len >= OSAL_ENCRYPT_BUFFER_SZ) break;
    }
#endif
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
#if 0
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
            if (osal_mbedtls_do_ssl_handshake(sslsocket) == OSAL_SSLSTATUS_FAIL)
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
            status = osal_mbedtls_get_sslstatus(sslsocket->ssl, (os_int)nprocessed);
            if (status == OSAL_SSLSTATUS_WANT_IO)
            {
                do {
                    n = BIO_read(sslsocket->wbio, buf, sizeof(buf));
                    if (n > 0)
                    {
                        osal_mbedtls_queue_encrypted_bytes(sslsocket, (char*)buf, (size_t)n);
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
#endif
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

  @brief Initialize the Mbed TLS library.
  @anchor osal_tls_initialize

  The osal_tls_initialize() initializes the underlying sockets library.

  @param   nic Pointer to array of network interface structures. Can be OS_NULL to use default.
  @param   n_nics Number of network interfaces in nic array.
  @param   prm Parameters related to TLS. Can OS_NULL for client if not needed.

  @return  None.

****************************************************************************************************
*/
void osal_tls_initialize(
    osalNetworkInterface2 *nic,
    os_int n_nics,
    osalSecurityConfig *prm)
{
    osal_socket_initialize(nic, n_nics);
    osal_mbedtls_init(prm);

    /* Set socket library initialized flag.
     */
    osal_tls_initialized = OS_TRUE;
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
    if (osal_tls_initialized)
    {
        osal_mbedtls_cleanup();
        osal_tls_initialized = OS_FALSE;
        osal_socket_shutdown();
    }
}


/**
****************************************************************************************************

  @brief Create and initialize SSL context.
  @anchor osal_mbedtls_init

  The osal_mbedtls_init() function sets up a SSL context.

  @return  None.

****************************************************************************************************
*/
static void osal_mbedtls_init(
    osalSecurityConfig *prm)
{
    int ret;
    const os_char personalization[] = "we could collect data from IO";

    /* Initialize random number generator
     */
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
        (const os_uchar*)personalization, os_strlen(personalization));
    if (ret != 0)
    {
        osal_debug_error_int("mbedtls_ctr_drbg_seed returned %d\n", ret);
    }
}


/**
****************************************************************************************************

  @brief Clean up Mbed TLS library.
  @anchor osal_mbedtls_cleanup

  The osal_mbedtls_cleanup() function...

  @param   sslsocket Stream pointer representing the SSL socket.
  @return  OSAL_SUCCESS if all fine. Other return values indicate an error.

****************************************************************************************************
*/
static void osal_mbedtls_cleanup(void)
{
    /* Free memory allocated for random number generator and entropy.
     */
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
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
