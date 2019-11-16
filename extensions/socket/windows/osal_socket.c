/**

  @file    socket/windows/osal_socket.c
  @brief   OSAL stream API implementation for windows sockets.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    15.11.2019

  Ethernet connectivity. Implementation of OSAL stream API and general network functionality
  using Windows sockets API. This implementation supports select functionality.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_SOCKET_SUPPORT

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <Ws2ipdef.h>
#include <Ws2tcpip.h>

/** Windows specific socket data structure. OSAL functions cast their own stream structure
    pointers to osalStream pointers.
 */
typedef struct osalSocket
{
    /** A stream structure must start with this generic stream header structure, which contains
        parameters common to every stream.
     */
    osalStreamHeader hdr;

	/** Operating system's socket handle.
	 */
	SOCKET handle;

#if OSAL_SOCKET_SELECT_SUPPORT
	/** Even to be set when new data has been received, can be sent, new connection has been 
		created, accepted or closed socket.
	 */
	WSAEVENT event;
#endif

	/** Stream open flags. Flags which were given to osal_socket_open() or osal_socket_accept()
        function. 
	 */
	os_int open_flags;

    /** This is IP v6 socket?
     */
    os_boolean is_ipv6;

    /** Ring buffer, OS_NULL if not used.
	 */
	os_uchar *buf;

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


/* Socket library initialized flag.
 */
os_boolean osal_sockets_initialized = OS_FALSE;


/* Prototypes for forward referred static functions.
 */
static osalStatus osal_socket_write2(
	osalSocket *mysocket,
    const os_char *buf,
	os_memsz n,
	os_memsz *n_written,
	os_int flags);

static void osal_socket_set_nodelay(
    SOCKET handle,
    DWORD state);

static void osal_socket_setup_ring_buffer(
	osalSocket *mysocket);

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
          - OSAL_STREAM_TCP_NODELAY: Disable Nagle's algorithm on TCP socket. Use TCP_CORK on
            linux, or TCP_NODELAY toggling on windows. If this flag is set, osal_socket_flush()
            must be called to actually transfer data.
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
	osalSocket *mysocket = OS_NULL;
    os_memsz sz1 = 0, sz2;
	os_int port_nr;
    os_char host[OSAL_HOST_BUF_SZ], nbuf[OSAL_NBUF_SZ];
    os_ushort *host_utf16 = OS_NULL, *port_utf16;
    ADDRINFOW *addrinfo = NULL;
    ADDRINFOW *ptr = NULL;
    ADDRINFOW hints;
	osalStatus rval;
	SOCKET handle = INVALID_SOCKET;
	struct sockaddr_in saddr;
    struct sockaddr_in6 saddr6;
    struct sockaddr *sa;
    void *sa_data;
    os_boolean is_ipv6;
    os_int af, udp, on = 1, s, sa_sz;

    /* If not initialized.
     */
    if (!osal_sockets_initialized)
    {
        if (status) *status = OSAL_STATUS_FAILED;
        return OS_NULL;
    }

	/* Get host name or numeric IP address and TCP port number from parameters.
	 */
    osal_socket_get_host_name_and_port(parameters, &port_nr, host, sizeof(host),
        &is_ipv6, flags, IOC_DEFAULT_SOCKET_PORT);
    udp = (flags & OSAL_STREAM_UDP_MULTICAST) ? OS_TRUE : OS_FALSE;

    af = is_ipv6 ? AF_INET6 : AF_INET;
    os_memclear(&hints, sizeof(hints));
    hints.ai_family = af;
    hints.ai_socktype = udp ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = udp ? IPPROTO_UDP : IPPROTO_TCP;

    if (is_ipv6)
    {
        os_memclear(&saddr6, sizeof(saddr6));
        os_memcpy(&saddr6.sin6_addr, &in6addr_any, sizeof(in6addr_any));
        saddr6.sin6_family = af;
        saddr6.sin6_port = htons(port_nr);
        sa = (struct sockaddr *)&saddr6;
        sa_sz = sizeof(saddr6);
        sa_data = &saddr6.sin6_addr.s6_addr;
    }
    else
    {
        os_memclear(&saddr, sizeof(saddr));
        saddr.sin_addr.s_addr = htonl(INADDR_ANY); 
        saddr.sin_family = af;
        saddr.sin_port = htons(port_nr);
        sa = (struct sockaddr *)&saddr;
        sa_sz = sizeof(saddr);
        sa_data = &saddr.sin_addr.s_addr;
    }
    
    if (host[0] != '\0')
    {
        host_utf16 = osal_str_utf8_to_utf16_malloc(host, &sz1);

        if (InetPtonW(af, host_utf16, sa_data) <= 0)
        {
            osal_int_to_str(nbuf, sizeof(nbuf), port_nr);
            port_utf16 = osal_str_utf8_to_utf16_malloc(nbuf, &sz2);

            s = GetAddrInfoW(host_utf16, port_utf16,
                &hints, &addrinfo);

            os_free(port_utf16, sz2);

            if (s || addrinfo == NULL) 
		    {
                if (addrinfo) FreeAddrInfoW(addrinfo);
			    rval = OSAL_STATUS_FAILED;
			    goto getout;
            }

            for (ptr = addrinfo; ptr != NULL; ptr = ptr->ai_next) 
            {
                if (ptr->ai_family == af) 
                {
                    os_memcpy(sa,  ptr->ai_addr, sa_sz);
                    break;
                }
            }

            FreeAddrInfoW(addrinfo);

            /* If no match found
             */
            if (ptr == NULL)
            {
			    rval = OSAL_STATUS_FAILED;
			    goto getout;
            }
	    }
    }

    /* Create socket.
     */
    handle = socket(af, hints.ai_socktype, hints.ai_protocol);
    if (handle == INVALID_SOCKET) 
	{
		rval = OSAL_STATUS_FAILED;
		goto getout;
	}

    /* Set socket reuse flag.
     */
    if ((flags & OSAL_STREAM_NO_REUSEADDR) == 0)
    {
        if (setsockopt(handle, SOL_SOCKET,  SO_REUSEADDR,
            (char *)&on, sizeof(on)) < 0)
        {
		    rval = OSAL_STATUS_FAILED;
		    goto getout;
        }
    }

	/* Set non blocking mode.
	 */
    if ((flags & OSAL_STREAM_BLOCKING) == 0)
    {
    	ioctlsocket(handle, FIONBIO, &on);
    }

	/* Allocate and clear socket structure.
	 */
	mysocket = (osalSocket*)os_malloc(sizeof(osalSocket), OS_NULL);
	if (mysocket == OS_NULL) 
	{
		rval = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
		goto getout;
	}
	os_memclear(mysocket, sizeof(osalSocket));

	/* Save socket handle and open flags.
	 */
	mysocket->handle = handle;
	mysocket->open_flags = flags;
    mysocket->is_ipv6 = is_ipv6;

	/* Save interface pointer.
	 */
	mysocket->hdr.iface = &osal_socket_iface;

#if OSAL_SOCKET_SELECT_SUPPORT
    /* If we are preparing to use this with select function.
     */
    if ((flags & (OSAL_STREAM_NO_SELECT|OSAL_STREAM_SELECT)) == OSAL_STREAM_SELECT)
    {   
        /* Create event
         */
        mysocket->event = WSACreateEvent();
        if (mysocket->event == WSA_INVALID_EVENT)
        {
		    rval = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
		    goto getout;
        }

        if (WSAEventSelect(handle, mysocket->event, FD_ACCEPT|FD_CONNECT|FD_CLOSE|FD_READ|FD_WRITE) == SOCKET_ERROR)
        {
		    rval = OSAL_STATUS_FAILED;
		    goto getout;
        }           
    }
#endif

	if (flags & (OSAL_STREAM_LISTEN | OSAL_STREAM_UDP_MULTICAST))
	{
		if (bind(handle, sa, sa_sz)) 
		{
			rval = OSAL_STATUS_FAILED;
			goto getout;
		}

        /* Set the listen back log
         */
	    if (flags & OSAL_STREAM_LISTEN)
            if (listen(handle, 32) , 0)
        {
		    rval = OSAL_STATUS_FAILED;
		    goto getout;
        }
	}

	else 
	{
		if (connect(handle, sa, sa_sz))
		{
            rval = WSAGetLastError();
            if (rval != WSAEWOULDBLOCK )
            {
			    rval = OSAL_STATUS_FAILED;
			    goto getout;
            }
		}

        /* If we work without Nagel.
         */
        if (flags & OSAL_STREAM_TCP_NODELAY)
        {
            osal_socket_setup_ring_buffer(mysocket);
        }
	}

	/* Release memory allocated for the host name or address.
	 */
    os_free(host_utf16, sz1);

	/* Success set status code and cast socket structure pointer to stream pointer and return it.
	 */
	if (status) *status = OSAL_SUCCESS;
	return (osalStream)mysocket;

getout:
	/* Opt out on error. First Release memory allocated for the host name or address.
	 */
    os_free(host_utf16, sz1);

    /* If we got far enough to allocate the socket structure.
       Close the event handle (if any) and free memory allocated
       for the socket structure.
     */
    if (mysocket)
    {
        if (mysocket->event) 
	    {
		    WSACloseEvent(mysocket->event);
	    }

        os_free(mysocket, sizeof(osalSocket));
    }

    /* Close socket
     */    
	if (handle != INVALID_SOCKET) 
	{
		closesocket(handle);
	}

	/* Set status code and return NULL pointer.
	 */
	if (status) *status = rval;
	return OS_NULL;
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
	SOCKET handle;
    char buf[64];
	os_int n, rval;

	/* If called with NULL argument, do nothing.
	 */
	if (stream == OS_NULL) return;

    /* Cast stream pointer to socket structure pointer, get operating system's socket handle.
	 */
	mysocket = (osalSocket*)stream;
    osal_debug_assert(mysocket->hdr.iface == &osal_socket_iface);
    handle = mysocket->handle;

    if (mysocket->event)
    {
        WSACloseEvent(mysocket->event);
    }

#if OSAL_DEBUG
    /* Mark socket closed
     */
    mysocket->hdr.iface = OS_NULL;
#endif

    /* Disable sending data. This informs other the end of socket that it is going down now.
     */
    if (shutdown(handle, SD_SEND))
    {
        rval = WSAGetLastError();
        if (rval != WSAENOTCONN)
        {
            osal_debug_error("shutdown() failed");
        }
    }

    /* Read data to be received until receive buffer is empty.
     */
    do
    {
        n = recv(handle, buf, sizeof(buf), 0);
        if (n == SOCKET_ERROR)
        {
#if OSAL_DEBUG
            rval = WSAGetLastError();
            if (rval != WSAEWOULDBLOCK && rval != WSAENOTCONN)
            {
                osal_debug_error("reading end failed");
            }
#endif
            break;
        }
    }
    while(n);

    /* Close the socket.
     */
    if (closesocket(handle))
    {
        osal_debug_error("closesocket failed");
    }

    /* Free ring buffer, if any
     */
	os_free(mysocket->buf, mysocket->buf_sz);

    /* Free memory allocated for socket structure.
     */
    os_free(mysocket, sizeof(osalSocket));
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
	osalSocket *mysocket, *newsocket = OS_NULL;
    SOCKET handle, new_handle = INVALID_SOCKET;
	os_int addr_size, on = 1;
	struct sockaddr_in sin_remote;
	struct sockaddr_in6 sin_remote6;
	osalStatus rval;

	if (stream)
	{
        /* Cast stream pointer to socket structure pointer, get operating system's socket handle.
         */
		mysocket = (osalSocket*)stream;
        osal_debug_assert(mysocket->hdr.iface == &osal_socket_iface);
        handle = mysocket->handle;

        /* Accept incoming connections.
		 */
        if (mysocket->is_ipv6)
        {
            addr_size = sizeof(sin_remote6);
            new_handle = accept(handle, (struct sockaddr*)&sin_remote6, &addr_size);
        }
        else
        {
            addr_size = sizeof(sin_remote);
            new_handle = accept(handle, (struct sockaddr*)&sin_remote, &addr_size);
        }

		/* If no new connection, do nothing more.
		 */
        if (new_handle == INVALID_SOCKET) 
		{
			if (status) *status = OSAL_STATUS_NO_NEW_CONNECTION;
			return OS_NULL;
		}

        /* Set socket reuse flag.
         */
        if ((flags & OSAL_STREAM_NO_REUSEADDR) == 0)
        {
            if (setsockopt(new_handle, SOL_SOCKET,  SO_REUSEADDR,
                (char *)&on, sizeof(on)) < 0)
            {
		        rval = OSAL_STATUS_FAILED;
		        goto getout;
            }
        }

	    /* Set non blocking mode.
	     */
        if ((flags & OSAL_STREAM_BLOCKING) == 0)
        {
    	    if (ioctlsocket(new_handle, FIONBIO, &on) == SOCKET_ERROR)
            {
		        rval = OSAL_STATUS_FAILED;
		        goto getout;
            }
        }

		/* Allocate and clear socket structure.
		 */
		newsocket = (osalSocket*)os_malloc(sizeof(osalSocket), OS_NULL);
		if (newsocket == OS_NULL) 
		{
			closesocket(new_handle);
			if (status) *status = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
			return OS_NULL;
		}
		os_memclear(newsocket, sizeof(osalSocket));

		/* Save socket handle and open flags.
		 */
		newsocket->handle = new_handle;
		newsocket->open_flags = flags;
        newsocket->is_ipv6 = mysocket->is_ipv6;

		/* Save interface pointer.
		 */
	#if OSAL_FUNCTION_POINTER_SUPPORT
		newsocket->hdr.iface = &osal_socket_iface;
	#endif

        /* If we work without Nagel.
         */
        if (flags & OSAL_STREAM_TCP_NODELAY)
        {
            osal_socket_setup_ring_buffer(newsocket);
        }

        /* If we are preparing to use this with select function.
         */
        if ((flags & (OSAL_STREAM_NO_SELECT|OSAL_STREAM_SELECT)) == OSAL_STREAM_SELECT)
        {   
            /* Create event
             */
            newsocket->event = WSACreateEvent();
            if (newsocket->event == WSA_INVALID_EVENT)
            {
		        rval = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
		        goto getout;
            }

            if (WSAEventSelect(new_handle, newsocket->event,
                FD_ACCEPT|FD_CONNECT|FD_CLOSE|FD_READ|FD_WRITE) == SOCKET_ERROR)
            {
		        rval = OSAL_STATUS_FAILED;
		        goto getout;
            }           
        }

        if (remote_ip_addr) *remote_ip_addr = '\0';

		/* Success set status code and cast socket structure pointer to stream pointer 
		   and return it.
		 */
		if (status) *status = OSAL_SUCCESS;
		return (osalStream)newsocket;
	}

getout:
	/* Opt out on error. If we got far enough to allocate the socket structure.
       Close the event handle (if any) and free memory allocated  for the socket structure.
     */
    if (newsocket)
    {
        if (newsocket->event) 
	    {
		    WSACloseEvent(newsocket->event);
	    }

        os_free(newsocket, sizeof(osalSocket));
    }

    /* Close socket
     */    
	if (new_handle != INVALID_SOCKET) 
	{
		closesocket(new_handle);
	}

	/* Set status code and return NULL pointer.
	 */
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

    if (stream)
    {
        mysocket = (osalSocket*)stream;
        head = mysocket->head;
        tail = mysocket->tail;
        if (head != tail)
        {
            if (head < tail) 
            {
                wrnow = mysocket->buf_sz - tail;

                osal_socket_set_nodelay(mysocket->handle, OS_TRUE);
                status = osal_socket_write2(mysocket, mysocket->buf + tail, wrnow, &nwr, flags);
                if (status) goto getout;
                if (nwr == wrnow) tail = 0;
                else tail += (os_short)nwr;
            }

            if (head > tail) 
            {
                wrnow = head - tail;

                osal_socket_set_nodelay(mysocket->handle, OS_TRUE);
                status = osal_socket_write2(mysocket, mysocket->buf + tail, wrnow, &nwr, flags);
                if (status) goto getout;
                tail += (os_short)nwr;
            }

            if (tail == head) 
            {
                tail = head = 0;
            }

            mysocket->head = head;
            mysocket->tail = tail;

        }
    }

	return OSAL_SUCCESS;

getout:
    return status;
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
static osalStatus osal_socket_write2(
	osalSocket *mysocket,
    const os_char *buf,
	os_memsz n,
	os_memsz *n_written,
	os_int flags)
{
	os_int rval, werr;
	SOCKET handle;
    osalStatus status;

    /* get operating system's socket handle.
		*/
	handle = mysocket->handle;

	rval = send(handle, buf, (int)n, 0);

	if (rval == SOCKET_ERROR)
	{
        werr = WSAGetLastError();

        /* WSAEWOULDBLOCK = operation would block.
            WSAENOTCONN = socket not (yet?) connected.
            */
        if (werr != WSAEWOULDBLOCK /* && werr != WSAENOTCONN */)
		{
            status = (werr == WSAECONNREFUSED)
                ? OSAL_STATUS_CONNECTION_REFUSED : OSAL_STATUS_FAILED;
	        *n_written = 0;
            return status;
		}
		rval = 0;
	}

	*n_written = rval;
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
    os_uchar *rbuf;
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

                    osal_socket_set_nodelay(mysocket->handle, OS_TRUE);
                    status = osal_socket_write2(mysocket, rbuf+tail, wrnow, &nwr, flags);
                    if (status) goto getout;
                    if (nwr == wrnow) tail = 0;
                    else tail += (os_short)nwr;
                }

                if (head > tail) 
                {
                    wrnow = head - tail;

                    osal_socket_set_nodelay(mysocket->handle, OS_TRUE);
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
    os_int rval, werr;
	osalSocket *mysocket;
	SOCKET handle;
    osalStatus status;

	if (stream)
	{
        /* Cast stream pointer to socket structure pointer, get operating system's socket handle.
		 */
		mysocket = (osalSocket*)stream;
        osal_debug_assert(mysocket->hdr.iface == &osal_socket_iface);
        handle = mysocket->handle;

        /* Check for errorneous arguments.
         */
        if (n < 0 || buf == OS_NULL)
        {
            status = OSAL_STATUS_FAILED;
            goto getout;
        }

		rval = recv(handle, buf, (int)n, 0);

        /* If other end has gracefylly closed.
         */
        if (rval == 0)
        {
            status = OSAL_STATUS_STREAM_CLOSED;
            goto getout;
        }

		if (rval == SOCKET_ERROR)
		{
            werr = WSAGetLastError();

            /* WSAEWOULDBLOCK = operation would block.
               WSAENOTCONN = socket not (yet?) connected. Important at least with TCP_NODELAY.
             */
            if (werr != WSAEWOULDBLOCK && werr != WSAENOTCONN)
			{
                status = (werr == WSAECONNREFUSED)
                    ? OSAL_STATUS_CONNECTION_REFUSED : OSAL_STATUS_FAILED;
				goto getout;
			}
			rval = 0;
		}

		*n_read = rval;
		return OSAL_SUCCESS;
	}
    status = OSAL_STATUS_FAILED;

getout:
	*n_read = 0;
    return status;
}


/**
****************************************************************************************************

  @brief Get socket parameter.
  @anchor osal_socket_get_parameter

  The osal_socket_get_parameter() function gets a parameter value.

  @param   stream Stream pointer representing the socket.
  @param   parameter_ix Index of parameter to get.
		   See @ref osalStreamParameterIx "stream parameter enumeration" for the list.
  @return  Parameter value.

****************************************************************************************************
*/
os_long osal_socket_get_parameter(
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
  @anchor osal_socket_set_parameter

  The osal_socket_set_parameter() function gets a parameter value.

  @param   stream Stream pointer representing the socket.
  @param   parameter_ix Index of parameter to get.
		   See @ref osalStreamParameterIx "stream parameter enumeration" for the list.
  @param   value Parameter value to set.
  @return  None.

****************************************************************************************************
*/
void osal_socket_set_parameter(
	osalStream stream,
	osalStreamParameterIx parameter_ix,
	os_long value)
{
	/* Call the default implementation
	 */
	osal_stream_default_set_parameter(stream, parameter_ix, value);
}


#if OSAL_SOCKET_SELECT_SUPPORT
/**
****************************************************************************************************

  @brief Wait for an event from one of sockets.
  @anchor osal_socket_select

  The osal_socket_select() function blocks execution of the calling thread until something
  happens with listed sockets, or or event given as argument is triggered.

  @param   streams Array of streams to wait for. These must be sockets, no mixing
           of different stream types is supported.
  @param   n_streams Number of stream pointers in "streams" array.
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
osalStatus osal_socket_select(
	osalStream *streams,
    os_int nstreams,
	osalEvent evnt,
	osalSelectData *selectdata,
    os_int timeout_ms,
    os_int flags)
{
	osalSocket *mysocket;
    osalSocket *sockets[OSAL_SOCKET_SELECT_MAX+1];
	WSAEVENT events[OSAL_SOCKET_SELECT_MAX+1];
	os_int ixtable[OSAL_SOCKET_SELECT_MAX+1];
	WSANETWORKEVENTS network_events;
    os_int i, n_sockets, n_events, event_nr, eventflags, errorcode;
    DWORD rval;
    
    os_memclear(selectdata, sizeof(osalSelectData));

    if (nstreams < 1 || nstreams > OSAL_SOCKET_SELECT_MAX)
        return OSAL_STATUS_FAILED;

    n_sockets = 0;
    for (i = 0; i < nstreams; i++)
    {
        mysocket = (osalSocket*)streams[i];
        if (mysocket)
        {
            osal_debug_assert(mysocket->hdr.iface == &osal_socket_iface);
            sockets[n_sockets] = mysocket;
            events[n_sockets] = mysocket->event;
            ixtable[n_sockets++] = i;
        }
    }
    n_events = n_sockets;

    /* If we have event, add it to wait.
     */
    if (evnt)
    {
        events[n_events++] = evnt;
    }

    rval = WSAWaitForMultipleEvents(n_events,
        events, FALSE, timeout_ms ? timeout_ms : WSA_INFINITE, FALSE);

    if (rval == WSA_WAIT_TIMEOUT)
    {
        selectdata->eventflags = OSAL_STREAM_TIMEOUT_EVENT;
        selectdata->stream_nr = OSAL_STREAM_NR_TIMEOUT_EVENT;
        return OSAL_SUCCESS;
    }

    event_nr = (os_int)(rval - WSA_WAIT_EVENT_0);

    if (evnt && event_nr == n_sockets)
    {
        selectdata->eventflags = OSAL_STREAM_CUSTOM_EVENT;
        selectdata->stream_nr = OSAL_STREAM_NR_CUSTOM_EVENT;
		return OSAL_SUCCESS;
    }

    if (event_nr < 0 || event_nr >= n_sockets)
    {
		return OSAL_STATUS_FAILED;
    }

    if (WSAEnumNetworkEvents(sockets[event_nr]->handle,
        events[event_nr], &network_events) == SOCKET_ERROR)
    {
		return OSAL_STATUS_FAILED;
    }

    eventflags = 0;
    errorcode = OSAL_SUCCESS;
	if (network_events.lNetworkEvents & FD_ACCEPT)
	{
        eventflags |= OSAL_STREAM_ACCEPT_EVENT;
        if (network_events.iErrorCode[FD_ACCEPT_BIT])
        {
            errorcode = OSAL_STATUS_FAILED;
        }
	}

	if (network_events.lNetworkEvents & FD_CONNECT)
	{
        eventflags |= OSAL_STREAM_CONNECT_EVENT;
        if (network_events.iErrorCode[FD_CONNECT_BIT])
        {
            eventflags |= OSAL_STREAM_CLOSE_EVENT;
            errorcode = OSAL_STATUS_STREAM_CLOSED;
        }
	}

	if (network_events.lNetworkEvents & FD_CLOSE)
	{
        eventflags |= OSAL_STREAM_CLOSE_EVENT;
        errorcode = OSAL_STATUS_STREAM_CLOSED;
        if (network_events.iErrorCode[FD_CLOSE_BIT])
        {
            errorcode = OSAL_STATUS_FAILED;
        }
	}

	if (network_events.lNetworkEvents & FD_READ)
	{
        eventflags |= OSAL_STREAM_READ_EVENT;
        if (network_events.iErrorCode[FD_READ_BIT])
        {
            errorcode = OSAL_STATUS_FAILED;
        }
	}

	if (network_events.lNetworkEvents & FD_WRITE)
	{
        eventflags |= OSAL_STREAM_WRITE_EVENT;
	}

    selectdata->eventflags = eventflags;
    selectdata->errorcode = errorcode;
    selectdata->stream_nr = ixtable[event_nr];

    return OSAL_SUCCESS;
}
#endif


/**
****************************************************************************************************

  @brief Initialize sockets.
  @anchor osal_socket_initialize

  The osal_socket_initialize() initializes the underlying sockets library.

  @param   nic Pointer to array of network interface structures. Ignored in Windows.
  @param   n_nics Number of network interfaces in nic array.
  @return  None.

****************************************************************************************************
*/
void osal_socket_initialize(
    osalNetworkInterface *nic,
    os_int n_nics)
{
    /** Windows socket library version information.
     */
    WSADATA osal_wsadata;

    /* Set socket library initialized flag.
     */
    osal_sockets_initialized = OS_TRUE;

	/* If socket library is already initialized, do nothing.
	 */
	if (osal_global->sockets_shutdown_func) return;

	/* Lock the system mutex to synchronize.
	 */
	os_lock();

	/* If socket library is already initialized, do nothing. Double checked here
	   for thread synchronization.
	 */
	if (!osal_global->sockets_shutdown_func) 
	{
		/* Initialize winsock.
		 */
		if (WSAStartup(MAKEWORD(2,2), &osal_wsadata))
		{
			osal_debug_error("WSAStartup() failed");
			return;
		}

		/* Mark that socket library has been initialized by setting shutdown function pointer.
           Now the pointer is shared on windows by main program and DLL. If this needs to
           be separated, move sockets_shutdown_func pointer from global structure to
           plain global variable.
		 */
		osal_global->sockets_shutdown_func = osal_socket_shutdown;
	}

	/* End synchronization.
	 */
	os_unlock();
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
	/* If socket library is not initialized, do nothing.
	 */
	if (!osal_global->sockets_shutdown_func) return;

	/* Initialize winsock.
	 */
	if (WSACleanup())
	{
		osal_debug_error("WSACleanup() failed");
		return;
	}

	/* Mark that socket library is no longer initialized.
	 */
    osal_global->sockets_shutdown_func = OS_NULL;
}

/**
****************************************************************************************************

  @brief Enable or disable nagle's algorithm.
  @anchor osal_socket_set_nodelay

  The osal_socket_set_nodelay() function contols use of Nagle's algorith. Nagle's algorithm is
  simple: wait for the peer to acknowledge the previously sent packet before sending any partial
  packets. This gives the OS time to coalesce multiple calls to write() from the application into
  larger packets before forwarding the data to the peer.

  @param  handle Socket handle.
  @param  state Nonzero to disable Nagle's algorithm (no delay mode), zero to enable it.
  @return None.

****************************************************************************************************
*/
static void osal_socket_set_nodelay(
    SOCKET handle,
    DWORD state)
{
    setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, (char*)&state, sizeof(state));
}


static void osal_socket_setup_ring_buffer(
	osalSocket *mysocket)
{
	mysocket->buf_sz = 1420; /* selected for TCP sockets */
	mysocket->buf = os_malloc(mysocket->buf_sz, OS_NULL);
}


#if OSAL_FUNCTION_POINTER_SUPPORT

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
	osal_socket_get_parameter,
	osal_socket_set_parameter,
#if OSAL_SOCKET_SELECT_SUPPORT
    osal_socket_select};
#else
    osal_stream_default_select};
#endif

#endif

#endif
