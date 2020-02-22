/**

  @file    socket/windows/osal_socket.c
  @brief   OSAL stream API implementation for windows sockets.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

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
#include <Ws2tcpip.h>


#include <iphlpapi.h>
#include <stdio.h> // testing
#include <stdlib.h> // testing

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
    os_memsz sz1 = 0;
	os_int port_nr;
    os_char addr[16];
	osalStatus rval; 
	SOCKET handle = INVALID_SOCKET;
	struct sockaddr_in saddr;
    struct sockaddr_in6 saddr6;
    struct sockaddr *sa;
    void *sa_data;
    os_boolean is_ipv6;
    os_int af, udp, on = 1, s, sa_sz;
    os_int info_code;
    struct ip_mreq mreq;

    /* If not initialized.
     */
    if (!osal_sockets_initialized)
    {
        if (status) *status = OSAL_STATUS_FAILED;
        return OS_NULL;
    }

	/* Get host name or numeric IP address and TCP port number from parameters.
	 */
    s = osal_socket_get_ip_and_port(parameters, addr, sizeof(addr),
        &port_nr, &is_ipv6, flags, IOC_DEFAULT_SOCKET_PORT);
    if (s)
    {
        if (status) *status = s;
        return OS_NULL;
    }

    udp = (flags & OSAL_STREAM_UDP_MULTICAST) ? OS_TRUE : OS_FALSE;

    if (is_ipv6)
    {
        af = AF_INET6;
        os_memclear(&saddr6, sizeof(saddr6));
        saddr6.sin6_family = af;
        saddr6.sin6_port = htons(port_nr);
        os_memcpy(&saddr6.sin6_addr, &addr, 16); 
        sa = (struct sockaddr *)&saddr6;
        sa_sz = sizeof(saddr6);
        sa_data = &saddr6.sin6_addr.s6_addr;
saddr.sin_addr.s_addr = 0; // DUMMY TO PREVENT COMPILER WARNING
    }
    else
    {
        af = AF_INET;
        os_memclear(&saddr, sizeof(saddr));
        saddr.sin_family = af;
        saddr.sin_port = htons(port_nr);
        os_memcpy(&saddr.sin_addr.s_addr, addr, 4);
        sa = (struct sockaddr *)&saddr;
        sa_sz = sizeof(saddr);
        sa_data = &saddr.sin_addr.s_addr;
    }

    /* Create socket.
     */
    handle = socket(af, udp ? SOCK_DGRAM : SOCK_STREAM,  udp ? IPPROTO_UDP : IPPROTO_TCP);
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
  	ioctlsocket(handle, FIONBIO, &on);

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

    if (flags & OSAL_STREAM_UDP_MULTICAST)
    {
        if (flags & OSAL_STREAM_LISTEN)
        {
            /* Use setsockopt to join a multicast group. Note that the socket should be bound to 
               the wildcard address (INADDR_ANY) before joining the group
             */
            os_memclear(&mreq, sizeof(mreq));
            if (inet_pton(AF_INET, option, &mreq.imr_multiaddr.s_addr) != 1) {
                osal_debug_error("osal_socket_open: inet_pton() failed");
            }

            mreq.imr_interface.s_addr = saddr.sin_addr.s_addr; // THIS IS IPv4 ONLY - IPv6 SUPPORT MISSING
mreq.imr_interface.s_addr = inet_addr("192.168.1.229");
            saddr.sin_addr.s_addr = INADDR_ANY;
            if (bind(handle, sa, sa_sz))
            {
                rval = OSAL_STATUS_FAILED;
                goto getout;
            }
            if (setsockopt(handle, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) < 0)
            {
                rval = OSAL_STATUS_UDP_MULTICAST_GROUP_FAILED;
                goto getout;
            }
        }
        info_code = OSAL_UDP_SOCKET_CONNECTED;
    }

	else if (flags & OSAL_STREAM_LISTEN)
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

        info_code = (mysocket->open_flags & OSAL_STREAM_UDP_MULTICAST)
            ? OSAL_UDP_SOCKET_CONNECTED : OSAL_LISTENING_SOCKET_CONNECTED;
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

        info_code = OSAL_SOCKET_CONNECTED;
    }

    /* Success, inform error handler, set status code and return stream pointer.
     */
    osal_info(eosal_mod, info_code, parameters);
    if (status) *status = OSAL_SUCCESS;
	return (osalStream)mysocket;

getout:
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
    osalStream stream,
    os_int flags)
{
	osalSocket *mysocket;
	SOCKET handle;
    char buf[64], nbuf[OSAL_NBUF_SZ];
    os_int n, rval, info_code;

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

    /* Report close info even if we report problem closing socket, we need
       keep count of sockets open correct.
     */
    osal_int_to_str(nbuf, sizeof(nbuf), handle);
    if (mysocket->open_flags & OSAL_STREAM_UDP_MULTICAST)
    {
        info_code = OSAL_UDP_SOCKET_DISCONNECTED;
    }
    else if (mysocket->open_flags & OSAL_STREAM_LISTEN)
    {
        info_code = OSAL_LISTENING_SOCKET_DISCONNECTED;
    }
    else
    {
        info_code = OSAL_SOCKET_DISCONNECTED;
    }
    osal_info(eosal_mod, info_code, nbuf);

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
        if (ioctlsocket(new_handle, FIONBIO, &on) == SOCKET_ERROR)
        {
            rval = OSAL_STATUS_FAILED;
            goto getout;
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
    os_int i, n_sockets, n_events, event_nr /* , eventflags, errorcode */;
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
        // selectdata->eventflags = OSAL_STREAM_TIMEOUT_EVENT;
        selectdata->stream_nr = OSAL_STREAM_NR_TIMEOUT_EVENT;
        return OSAL_SUCCESS;
    }

    event_nr = (os_int)(rval - WSA_WAIT_EVENT_0);

    if (evnt && event_nr == n_sockets)
    {
        // selectdata->eventflags = OSAL_STREAM_CUSTOM_EVENT;
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

    /* eventflags = 0;
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
	} */

    // selectdata->eventflags = eventflags;
    // selectdata->errorcode = errorcode;
    selectdata->stream_nr = ixtable[event_nr];

    return OSAL_SUCCESS;
}
#endif


/**
****************************************************************************************************

  @brief Write packet (UDP) to stream.
  @anchor osal_socket_send_packet

  The osal_socket_send_packet() function writes UDP packet to network.

  @param   stream Stream pointer representing the UDP socket.
  @param   parameters IP address and optionallt port where to send the UDP message.
  @param   buf Pointer to the beginning of data to send.
  @param   n Number of bytes to send.
  @param   flags Set OSAL_STREAM_DEFAULT.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates that packet was written.
           Value OSAL_PENDING that we network is too bysy for the moment. Other return values
           indicate an error.

****************************************************************************************************
*/
osalStatus osal_socket_send_packet(
    osalStream stream,
    const os_char *parameters,
    const os_char *buf,
    os_memsz n,
    os_int flags)
{
    osalSocket *mysocket;
    struct sockaddr_in sin_remote;
    struct sockaddr_in6 sin_remote6;
    os_char addr[16];
    os_int port_nr;
    int nbytes, werr;
    osalStatus s;
    os_boolean is_ipv6;

    if (stream == OS_NULL) return OSAL_STATUS_FAILED;

    /* Cast stream pointer to socket structure pointer.
     */
    mysocket = (osalSocket*)stream;
    osal_debug_assert(mysocket->hdr.iface == &osal_socket_iface);

    s = osal_socket_get_ip_and_port(parameters, addr, sizeof(addr),
        &port_nr, &is_ipv6, flags, IOC_DEFAULT_SOCKET_PORT);
    if (s) return s;

    if (mysocket->is_ipv6)
    {
        /* Set up destination address
         */
        os_memclear(&sin_remote6, sizeof(sin_remote6));
        sin_remote6.sin6_family = AF_INET6;
        sin_remote6.sin6_port = htons(port_nr);
        memcpy(&sin_remote6.sin6_addr, &addr, sizeof(in6addr_any));

        /* Send packet.
         */
        nbytes = sendto(mysocket->handle, buf, (int)n, 0,
            (struct sockaddr*)&sin_remote6, sizeof(sin_remote6));
    }
    else
    {
        /* Set up destination address
         */
        os_memclear(&sin_remote, sizeof(sin_remote));
        sin_remote.sin_family = AF_INET;
        sin_remote.sin_port = htons(port_nr);
        memcpy(&sin_remote.sin_addr.s_addr, addr, 4);

        /* Send packet.
         */
        nbytes = sendto(mysocket->handle, buf, (int)n, 0,
            (struct sockaddr*)&sin_remote, sizeof(sin_remote));
    }

    if (nbytes < 0)
    {
        werr = WSAGetLastError();

        /* WSAEWOULDBLOCK = operation would block.
           WSAENOTCONN = socket not (yet?) connected.
         */
        return (werr == WSAEWOULDBLOCK)
            ? OSAL_PENDING : OSAL_STATUS_FAILED;
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Read packet (UDP) from stream.
  @anchor osal_socket_receive_packet

  The osal_socket_receive_packet() function read UDP packet from network. Function never blocks.

  @param   stream Stream pointer representing the UDP socket.
  @param   buf Pointer to buffer where to read data.
  @param   n Buffer size in bytes.
  @param   n_read Number of bytes actually read.
  @param   remote_address Pointer to string buffer into which to store the IP address
           from which the incoming connection was accepted. Can be OS_NULL if not needed.
  @param   remote_addr_sz Size of remote IP address buffer in bytes.
  @param   flags Set OSAL_STREAM_DEFAULT.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates that packet was read.
           Value OSAL_PENDING that we we have no received UDP message to read for the moment.
           Other return values indicate an error.

****************************************************************************************************
*/
osalStatus osal_socket_receive_packet(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_char *remote_addr,
    os_memsz remote_addr_sz,
    os_int flags)
{
    osalSocket *mysocket;
    struct sockaddr_in sin_remote;
    // struct sockaddr_storage sin_remote;
    struct sockaddr_in6 sin_remote6;
    int nbytes, werr;
    socklen_t addr_size;
    char addrbuf[INET6_ADDRSTRLEN];

    if (n_read) *n_read = 0;
    if (remote_addr) *remote_addr = '\0';

    if (stream == OS_NULL) return OSAL_STATUS_FAILED;

    /* Cast stream pointer to socket structure pointer.
     */
    mysocket = (osalSocket*)stream;
    osal_debug_assert(mysocket->hdr.iface == &osal_socket_iface);

    /* Try to get UDP packet from incoming socket.
     */
    if (mysocket->is_ipv6)
    {
        addr_size = sizeof(sin_remote6);
        nbytes = recvfrom(mysocket->handle, buf, (int)n, 0,
            (struct sockaddr*)&sin_remote6, &addr_size);
    }
    else
    {
        addr_size = sizeof(sin_remote);
        nbytes = recvfrom(mysocket->handle, buf, (int)n, 0,
            (struct sockaddr*)&sin_remote, &addr_size);
    }

    if (nbytes < 0)
    {
        werr = WSAGetLastError();

        /* WSAEWOULDBLOCK = operation would block.
            WSAENOTCONN = socket not (yet?) connected.
            */
        return (werr == WSAEWOULDBLOCK)
            ? OSAL_PENDING : OSAL_STATUS_FAILED;
    }

    if (remote_addr)
    {
        if (mysocket->is_ipv6)
        {
            inet_ntop(AF_INET6, &sin_remote6.sin6_addr, addrbuf, sizeof(addrbuf));
        }
        else
        {
            inet_ntop(AF_INET, &sin_remote.sin_addr, addrbuf, sizeof(addrbuf));
        }
        os_strncpy(remote_addr, addrbuf, remote_addr_sz);
    }

    if (n_read) *n_read = nbytes;
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief List network interfaces which can be used for UDP multicasts.
  @anchor osal_socket_list_network_interfaces

  The osal_socket_list_network_interfaces() function read UDP packet from network. Function never blocks.

  @param   stream Stream pointer representing the UDP socket.
  @param   buf Pointer to buffer where to read data.
  @param   n Buffer size in bytes.
  @param   n_read Number of bytes actually read.
  @param   remote_address Pointer to string buffer into which to store the IP address
           from which the incoming connection was accepted. Can be OS_NULL if not needed.
  @param   remote_addr_sz Size of remote IP address buffer in bytes.
  @param   flags Set OSAL_STREAM_DEFAULT.
  @return  Function status code. Value OSAL_SUCCESS (0) indicates that packet was read.
           Value OSAL_PENDING that we we have no received UDP message to read for the moment.
           Other return values indicate an error.

****************************************************************************************************
*/
osalStatus osal_socket_list_network_interfaces(
    os_int xxxx)
{

// Link with Iphlpapi.lib
#pragma comment(lib, "IPHLPAPI.lib")

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

    /* Declare and initialize variables */

    DWORD dwSize = 0;
    DWORD dwRetVal = 0;

    unsigned int i = 0;

    // Set the flags to pass to GetAdaptersAddresses
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;

    // default to unspecified address family (both)
    ULONG family = AF_UNSPEC;

    LPVOID lpMsgBuf = NULL;

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    ULONG outBufLen = 0;
    ULONG Iterations = 0;

    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
    PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
    PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
    IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;
    IP_ADAPTER_PREFIX *pPrefix = NULL;

family = AF_INET;
family = AF_INET6;
family = AF_UNSPEC; // Both IPv4 and IPv6

    // Allocate a 15 KB buffer to start with.
    outBufLen = WORKING_BUFFER_SIZE;

    do {

        pAddresses = (IP_ADAPTER_ADDRESSES *) MALLOC(outBufLen);
        if (pAddresses == NULL) {
            printf
                ("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
            exit(1);
        }

        dwRetVal =
            GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            FREE(pAddresses);
            pAddresses = NULL;
        } else {
            break;
        }

        Iterations++;

    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));

    if (dwRetVal == NO_ERROR) {
        // If successful, output some information from the data we received
        pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            printf("\tLength of the IP_ADAPTER_ADDRESS struct: %ld\n",
                   pCurrAddresses->Length);
            printf("\tIfIndex (IPv4 interface): %u\n", pCurrAddresses->IfIndex);
            printf("\tAdapter name: %s\n", pCurrAddresses->AdapterName);

            pUnicast = pCurrAddresses->FirstUnicastAddress;
            if (pUnicast != NULL) {
                for (i = 0; pUnicast != NULL; i++)
                    pUnicast = pUnicast->Next;
                printf("\tNumber of Unicast Addresses: %d\n", i);
            } else
                printf("\tNo Unicast Addresses\n");

            pAnycast = pCurrAddresses->FirstAnycastAddress;
            if (pAnycast) {
                for (i = 0; pAnycast != NULL; i++)
                    pAnycast = pAnycast->Next;
                printf("\tNumber of Anycast Addresses: %d\n", i);
            } else
                printf("\tNo Anycast Addresses\n");

            pMulticast = pCurrAddresses->FirstMulticastAddress;
            if (pMulticast) {
                for (i = 0; pMulticast != NULL; i++)
                    pMulticast = pMulticast->Next;
                printf("\tNumber of Multicast Addresses: %d\n", i);
            } else
                printf("\tNo Multicast Addresses\n");

            pDnServer = pCurrAddresses->FirstDnsServerAddress;
            if (pDnServer) {
                for (i = 0; pDnServer != NULL; i++)
                    pDnServer = pDnServer->Next;
                printf("\tNumber of DNS Server Addresses: %d\n", i);
            } else
                printf("\tNo DNS Server Addresses\n");

            printf("\tDNS Suffix: %wS\n", pCurrAddresses->DnsSuffix);
            printf("\tDescription: %wS\n", pCurrAddresses->Description);
            printf("\tFriendly name: %wS\n", pCurrAddresses->FriendlyName);

            if (pCurrAddresses->PhysicalAddressLength != 0) {
                printf("\tPhysical address: ");
                for (i = 0; i < (int) pCurrAddresses->PhysicalAddressLength;
                     i++) {
                    if (i == (pCurrAddresses->PhysicalAddressLength - 1))
                        printf("%.2X\n",
                               (int) pCurrAddresses->PhysicalAddress[i]);
                    else
                        printf("%.2X-",
                               (int) pCurrAddresses->PhysicalAddress[i]);
                }
            }
            printf("\tFlags: %ld\n", pCurrAddresses->Flags);
            printf("\tMtu: %lu\n", pCurrAddresses->Mtu);
            printf("\tIfType: %ld\n", pCurrAddresses->IfType);
            printf("\tOperStatus: %ld\n", pCurrAddresses->OperStatus);
            printf("\tIpv6IfIndex (IPv6 interface): %u\n",
                   pCurrAddresses->Ipv6IfIndex);
            printf("\tZoneIndices (hex): ");
            for (i = 0; i < 16; i++)
                printf("%lx ", pCurrAddresses->ZoneIndices[i]);
            printf("\n");

            printf("\tTransmit link speed: %I64u\n", pCurrAddresses->TransmitLinkSpeed);
            printf("\tReceive link speed: %I64u\n", pCurrAddresses->ReceiveLinkSpeed);

            pPrefix = pCurrAddresses->FirstPrefix;
            if (pPrefix) {
                for (i = 0; pPrefix != NULL; i++)
                    pPrefix = pPrefix->Next;
                printf("\tNumber of IP Adapter Prefix entries: %d\n", i);
            } else
                printf("\tNumber of IP Adapter Prefix entries: 0\n");

            printf("\n");

            pCurrAddresses = pCurrAddresses->Next;
        }
    } else {
        printf("Call to GetAdaptersAddresses failed with error: %d\n",
               dwRetVal);
        if (dwRetVal == ERROR_NO_DATA)
            printf("\tNo addresses were found for the requested parameters\n");
        else {

            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
                    NULL, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),   
                    // Default language
                    (LPTSTR) & lpMsgBuf, 0, NULL)) {
                printf("\tError: %s", lpMsgBuf);
                LocalFree(lpMsgBuf);
                if (pAddresses)
                    FREE(pAddresses);
                exit(1);
            }
        }
    }

    if (pAddresses) {
        FREE(pAddresses);
    }

    return 0;
}

/**
****************************************************************************************************

  @brief Initialize sockets.
  @anchor osal_socket_initialize

  The osal_socket_initialize() initializes the underlying sockets library.

  @param   nic Pointer to array of network interface structures. Ignored in Windows.
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

  @brief Check if network is initialized.
  @anchor osal_are_sockets_initialized

  The osal_are_sockets_initialized function is called to check if socket library initialization
  has been completed. For Windows there is not much to do, we just return if sockect library
  has been initialized.

  @return  OSAL_SUCCESS if we are connected to a wifi network.
           OSAL_PENDING If currently connecting and have not never failed to connect so far.
           OSAL_STATUS_FALED No network, at least for now.

****************************************************************************************************
*/
osalStatus osal_are_sockets_initialized(
    void)
{
    return osal_global->sockets_shutdown_func ? OSAL_SUCCESS : OSAL_STATUS_FAILED;
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


const osalStreamInterface osal_socket_iface
 = {OSAL_STREAM_IFLAG_NONE,
    osal_socket_open,
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
    osal_socket_select,
#else
    osal_stream_default_select,
#endif
    osal_socket_send_packet,
    osal_socket_receive_packet};

#endif
