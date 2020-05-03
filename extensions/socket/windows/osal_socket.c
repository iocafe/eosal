/**

  @file    socket/windows/osal_socket.c
  @brief   OSAL stream API implementation for windows sockets.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    24.2.2020

  Ethernet connectivity. Implementation of OSAL stream API and general network functionality
  using Windows sockets API. This implementation supports select functionality.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/

/* Impoetant: WIN32_LEAN_AND_MEAN must be defined to use winsock2! Do this before including
   eosalx.h, because it further includes Windows headers.
*/
#define WIN32_LEAN_AND_MEAN

#include "eosalx.h"
#if OSAL_SOCKET_SUPPORT

#include <winsock2.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>

#include "extensions/net/common/osal_shared_net_info.h"

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

    /** Multicast group address (binary)
     */
    os_char multicast_group[OSAL_IP_BIN_ADDR_SZ];

    /** Network interface list to for sending multicasts. Interface numbers for IPv6
        For IPv4 list of interface addressess.
     */
    os_char *send_mcast_ifaces;
    os_int send_mcast_ifaces_n;
    os_int send_mcast_ifaces_sz;

    /** Port number for multicasts or listening connections.
     */
    os_int passive_port;

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


typedef union osalSocketAddress
{
    struct sockaddr_in ip4;
    struct sockaddr_in6 ip6;
}
osalSocketAddress;


/* Prototypes for forward referred static functions.
 */
static osalStatus osal_setup_tcp_socket(
    osalSocket *mysocket,
    os_char *iface_addr_bin,
    os_boolean iface_addr_is_ipv6,
    os_int port_nr,
    os_int flags);

static osalStatus osal_setup_socket_for_udp_multicasts(
    osalSocket *mysocket,
    os_char *multicast_group_addr_str,
    os_char *iface_addr_bin,
    os_boolean iface_addr_is_ipv6,
    os_int port_nr,
    os_int flags);

static osalStatus osal_socket_alloc_send_mcast_ifaces(
    osalSocket *mysocket,
    os_int n);

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

static os_int osal_socket_list_network_interfaces(
    osalStream interface_list,
    os_uint family,
    os_boolean get_interface_index);

static os_int osal_get_interface_index_by_ipv6_address(
    os_char *iface_list_str,
    os_char *iface_addr_bin);


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

  @param  option UDP multicasts, the multicast group address as string.
          Otherwise not used for sockets, set OS_NULL.

  @param  status Pointer to integer into which to store the function status code. Value
          OSAL_SUCCESS (0) indicates success and all nonzero values indicate an error.
          See @ref osalStatus "OSAL function return codes" for full list.
          This parameter can be OS_NULL, if no status code is needed.

  @param  flags Flags for creating the socket. Bit fields, combination of:
          - OSAL_STREAM_CONNECT: Connect to specified socket port at specified IP address.
          - OSAL_STREAM_LISTEN: Open a socket to listen for incoming connections.
          - OSAL_STREAM_MULTICAST: Open a UDP multicast socket.
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
    os_char iface_addr_bin[OSAL_IP_BIN_ADDR_SZ];
    os_int port_nr;
    os_boolean is_ipv6;
    osalStatus s;
    os_int info_code;

    /* Return OS_NULL if network not (yet) initialized initialized.
     */
    s = osal_are_sockets_initialized();
    if (s)
    {
        if (status) *status = s;
        return OS_NULL;
    }

    /* Get host name or numeric IP address and TCP port number from parameters.
     */
    s = osal_socket_get_ip_and_port(parameters, iface_addr_bin, sizeof(iface_addr_bin),
        &port_nr, &is_ipv6, flags, IOC_DEFAULT_SOCKET_PORT);
    if (s)
    {
        if (status) *status = s;
        return OS_NULL;
    }

    /* Allocate and clear socket structure.
     */
    mysocket = (osalSocket*)os_malloc(sizeof(osalSocket), OS_NULL);
    if (mysocket == OS_NULL)
    {
        s = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        goto getout;
    }
    os_memclear(mysocket, sizeof(osalSocket));

    /* Save socket open flags and interface pointer.
     */
    mysocket->open_flags = flags;
    mysocket->hdr.iface = &osal_socket_iface;

    /* Open UDP multicast socket
     */
    if (flags & OSAL_STREAM_MULTICAST)
    {
        s = osal_setup_socket_for_udp_multicasts(mysocket, option,
            iface_addr_bin, is_ipv6, port_nr, flags);
        if (s) goto getout;
        info_code = OSAL_UDP_SOCKET_CONNECTED;
    }

    /* Open TCP socket.
     */
    else
    {
        s = osal_setup_tcp_socket(mysocket, iface_addr_bin, is_ipv6, port_nr, flags);
        if (s) goto getout;
        info_code = (flags & OSAL_STREAM_LISTEN)
            ? OSAL_LISTENING_SOCKET_CONNECTED: OSAL_SOCKET_CONNECTED;
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
        /* Close socket
         */
        if (mysocket->handle != INVALID_SOCKET)
        {
            closesocket(mysocket->handle);
        }

        if (mysocket->event)
        {
            WSACloseEvent(mysocket->event);
        }

        /* Release memory allocated for multicast iface list.
         */
        osal_socket_alloc_send_mcast_ifaces(mysocket, 0);

        /* Free ring buffer, if any, and the socket structure.
         */
        os_free(mysocket->buf, mysocket->buf_sz);
        os_free(mysocket, sizeof(osalSocket));
    }

    /* Set status code and return NULL pointer.
     */
    if (status) *status = s;
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Connect or listen for TCP socket (internal).
  @anchor osal_setup_tcp_socket

  The osal_setup_tcp_socket() function....

  @param  mysocket Pointer to my socket structure.
  @param  iface_addr_bin IP address of network interface to use, binary format, 4 bytes for IPv4
          and 16 bytes for IPv6.
  @param  iface_addr_is_ipv6 OS_TRUE for IPv6, or OS_FALSE for IPv4.
  @param  port_nr TCP port number to listen or connect to.
  @param  flags Flags given to osal_socket_open().

  @return OSAL_SUCCESS (0) if all fine.

****************************************************************************************************
*/
static osalStatus osal_setup_tcp_socket(
    osalSocket *mysocket,
    os_char *iface_addr_bin,
    os_boolean iface_addr_is_ipv6,
    os_int port_nr,
    os_int flags)
{
    osalStatus s;
    SOCKET handle = INVALID_SOCKET;
    struct sockaddr_in saddr;
    struct sockaddr_in6 saddr6;
    struct sockaddr *sa;
    os_int af, sa_sz;
    int on;
    BOOL bon;

    if (iface_addr_is_ipv6)
    {
        af = AF_INET6;
        os_memclear(&saddr6, sizeof(saddr6));
        saddr6.sin6_family = af;
        saddr6.sin6_port = htons(port_nr);
        os_memcpy(&saddr6.sin6_addr, iface_addr_bin, OSAL_IPV6_BIN_ADDR_SZ);
        sa = (struct sockaddr *)&saddr6;
        sa_sz = sizeof(saddr6);
    }
    else
    {
        af = AF_INET;
        os_memclear(&saddr, sizeof(saddr));
        saddr.sin_family = af;
        saddr.sin_port = htons(port_nr);
        os_memcpy(&saddr.sin_addr.s_addr, iface_addr_bin, OSAL_IPV4_BIN_ADDR_SZ);
        sa = (struct sockaddr *)&saddr;
        sa_sz = sizeof(saddr);
    }

    /* Create socket.
     */
    handle = socket(af, SOCK_STREAM,  IPPROTO_TCP);
    if (handle == INVALID_SOCKET)
    {
        s = OSAL_STATUS_FAILED;
        goto getout;
    }

    /* Set socket reuse flag.
     */
    if ((flags & OSAL_STREAM_NO_REUSEADDR) == 0)
    {
        on = 1;
        if (setsockopt(handle, SOL_SOCKET, SO_REUSEADDR,
            (char *)&on, sizeof(on)) < 0)
        {
            s = OSAL_STATUS_FAILED;
            goto getout;
        }
    }

    /* Set non blocking mode.
     */
    on = 1;
    ioctlsocket(handle, FIONBIO, &on);
    bon = 1;
    setsockopt(handle, SOL_SOCKET, SO_DONTLINGER, (char *)&bon, sizeof(bon));

    /* Save flags and interface pointer.
     */
    mysocket->open_flags = flags;
    mysocket->is_ipv6 = iface_addr_is_ipv6;
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
            s = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
            goto getout;
        }

        if (WSAEventSelect(handle, mysocket->event, FD_ACCEPT|FD_CONNECT|FD_CLOSE|FD_READ|FD_WRITE)
            == SOCKET_ERROR)
        {
            s = OSAL_STATUS_FAILED;
            goto getout;
        }
    }
#endif

    if (flags & OSAL_STREAM_LISTEN)
    {
        if (bind(handle, sa, sa_sz))
        {
            s = OSAL_STATUS_FAILED;
            goto getout;
        }

        /* Set the listen back log
         */
        if (listen(handle, 32) != 0)
        {
            s = OSAL_STATUS_FAILED;
            goto getout;
        }

        /* Set any nonzero multicast port to indicate to close() function
         * that we do not need to call gracefull connection shutdown stuff.
         */
        mysocket->passive_port = port_nr;
    }

    else
    {
        if (connect(handle, sa, sa_sz))
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                s = OSAL_STATUS_FAILED;
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

    mysocket->handle = handle;
    return OSAL_SUCCESS;

getout:
    /* Close socket
     */
    if (handle != INVALID_SOCKET)
    {
        closesocket(handle);
    }

    /* Return status code
     */
    return s;
}


/**
****************************************************************************************************

  @brief Setup a socket either for sending or receiving UDP multicasts (internal).
  @anchor osal_setup_socket_for_udp_multicasts

  The osal_setup_socket_for_udp_multicasts() function....

  @param  mysocket Pointer to my socket structure.
  @param  multicast_group_addr_str The multicast group IP address as string.
  @param  iface_addr_bin IP address of network interface to use, binary format, 4 bytes for IPv4
          and 16 bytes for IPv6.
  @param  iface_addr_is_ipv6 OS_TRUE for IPv6, or OS_FALSE for IPv4.
  @param  port_nr UDP port number to listen or send multicasts to.
  @param  flags Flags given to osal_socket_open().

  @return OSAL_SUCCESS (0) if all fine.

****************************************************************************************************
*/
static osalStatus osal_setup_socket_for_udp_multicasts(
    osalSocket *mysocket,
    os_char *multicast_group_addr_str,
    os_char *iface_addr_bin,
    os_boolean iface_addr_is_ipv6,
    os_int port_nr,
    os_int flags)
{
    osalSocketGlobal *sg;
    SOCKET handle = INVALID_SOCKET;
    osalSocketAddress sin;
    struct ip_mreq mreq;
    struct ipv6_mreq mreq6;
    os_char ipbuf[OSAL_IPADDR_SZ], *p, *e;
    os_char nic_addr[OSAL_IP_BIN_ADDR_SZ];
    os_int i, n, ni;
    os_boolean has_iface_addr;
    os_int tmp_port_nr;
    os_boolean opt_is_ipv6, nic_is_ipv6;
    os_int af;
    osalStatus s;
    int on = 1;
    char *mr;
    os_int mr_sz, interface_ix, n_ifaces;
    osalStream interface_list = OS_NULL;
    os_char *iface_list_str = OS_NULL;
    BOOL bon;

    /* Save multicast port number
     */
    mysocket->passive_port = port_nr;

    /* Get global socket data.
     */
    sg = osal_global->socket_global;

    /* Is interface address given as function parameter? Set "has_iface_addr" to indicate.
     */
    n = iface_addr_is_ipv6 ? OSAL_IPV6_BIN_ADDR_SZ : OSAL_IPV4_BIN_ADDR_SZ;
    for (i = 0; i < n; i++) {
        if (iface_addr_bin[i]) break;
    }
    has_iface_addr = (os_boolean)(i != n);

    /* Get multicast group IP address from original "options" argument.
     */
    s = osal_socket_get_ip_and_port(multicast_group_addr_str, mysocket->multicast_group,
        OSAL_IP_BIN_ADDR_SZ, &tmp_port_nr, &opt_is_ipv6, flags, IOC_DEFAULT_SOCKET_PORT);
    if (s) return s;
    mysocket->is_ipv6 = opt_is_ipv6;

    /* Check that multicast and interface addresses (if given) as argument belong to the same
       address family. If there is conflict, issue error and use multicart group ip family
       and ignore interface address.
     */
    if (opt_is_ipv6 != iface_addr_is_ipv6)
    {
        if (has_iface_addr)
        {
            osal_debug_error_str("osal_socket_open UDP multicast and iface address family mismatch:",
                multicast_group_addr_str);
            has_iface_addr = OS_FALSE;
        }
    }

    /* Set address family and prepare socket address structure for listening UDP multicasts:
        port number set, but IP not bound to any specific network interface.
     */
    os_memclear(&sin, sizeof(sin));
    if (opt_is_ipv6)
    {
        af = AF_INET6;
        sin.ip6.sin6_family = AF_INET6;
        sin.ip6.sin6_port = htons(port_nr);
        sin.ip6.sin6_addr = in6addr_any;
    }
    else
    {
        af = AF_INET;
        sin.ip4.sin_family = AF_INET;
        sin.ip4.sin_port = htons(port_nr);
        sin.ip4.sin_addr.s_addr = INADDR_ANY;
    }

    /* Create socket.
     */
    handle = socket(af, SOCK_DGRAM, IPPROTO_UDP);
    if (handle == INVALID_SOCKET)
    {
        s = OSAL_STATUS_FAILED;
        goto getout;
    }

    /* Set socket reuse flag.
     */
    if ((flags & OSAL_STREAM_NO_REUSEADDR) == 0)
    {
        on = 1;
        if (setsockopt(handle, SOL_SOCKET, SO_REUSEADDR,
            (char *)&on, sizeof(on)) < 0)
        {
            s = OSAL_STATUS_FAILED;
            goto getout;
        }
    }

    /* Set non blocking mode.
     */
    on = 1;
    ioctlsocket(handle, FIONBIO, &on);
    bon = 1;
    setsockopt(handle, SOL_SOCKET, SO_DONTLINGER, (char *)&bon, sizeof(bon));

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
            s = OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
            goto getout;
        }

        if (WSAEventSelect(handle, mysocket->event,
            FD_ACCEPT|FD_CONNECT|FD_CLOSE|FD_READ|FD_WRITE) == SOCKET_ERROR)
        {
            s = OSAL_STATUS_FAILED;
            goto getout;
        }
    }
#endif

    /* Listen for multicasts.
     */
    if (flags & OSAL_STREAM_LISTEN)
    {
        /* Bind the socket, here we never bind to specific interface or IP.
         */
        if (bind(handle, (const struct sockaddr *)&sin, opt_is_ipv6
            ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in)))
        {
            s = OSAL_STATUS_FAILED;
            goto getout;
        }

        /* We need interface list to convert adapter addressess to adapter indices
         */
        if (af == AF_INET6)
        {
            interface_list = osal_stream_buffer_open(OS_NULL, OS_NULL, OS_NULL, OSAL_STREAM_DEFAULT);
            osal_socket_list_network_interfaces(interface_list, af, OS_TRUE);
            iface_list_str = osal_stream_buffer_content(interface_list, OS_NULL);
        }

        /* Inititalize a request to join to a multicast group.
         */
        if (opt_is_ipv6)
        {
            mr = (char*)&mreq6;
            mr_sz = sizeof(mreq6);
            os_memclear(&mreq6, sizeof(mreq6));
            os_memcpy(&mreq6.ipv6mr_multiaddr, mysocket->multicast_group, OSAL_IPV6_BIN_ADDR_SZ);
        }
        else
        {
            mr = (char*)&mreq;
            mr_sz = sizeof(mreq);
            os_memclear(&mreq, sizeof(mreq));
            os_memcpy(&mreq.imr_multiaddr.s_addr, mysocket->multicast_group, OSAL_IPV4_BIN_ADDR_SZ);
        }

        if (has_iface_addr)
        {
            if (opt_is_ipv6)
            {
                interface_ix = osal_get_interface_index_by_ipv6_address(iface_list_str, iface_addr_bin);
                if (interface_ix >= 0)
                {
                    mreq6.ipv6mr_interface = interface_ix;
                    if (setsockopt(handle, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char*)&mreq6, sizeof(mreq6)) < 0)
                    {
                        s = OSAL_STATUS_MULTICAST_GROUP_FAILED;
                        goto getout;
                    }
                }
                else
                {
                    has_iface_addr = OS_FALSE;
                    osal_debug_error("osal_setup_socket_for_udp_multicasts: Multicast source iface not found");
                }
            }
            else
            {
                os_memcpy(&mreq.imr_interface.s_addr, iface_addr_bin, OSAL_IPV4_BIN_ADDR_SZ);
                if (setsockopt(handle, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) mr, mr_sz) < 0)
                {
                    s = OSAL_STATUS_MULTICAST_GROUP_FAILED;
                    goto getout;
                }
            }
        }

        /* Address not a function parameter, see if we have it for the NIC
         */
        if (!has_iface_addr && (flags & OSAL_STREAM_USE_GLOBAL_SETTINGS))
        {
            for (i = 0; i < sg->n_nics; i++)
            {
                if (!sg->nic[i].receive_udp_multicasts) continue;

                s = osal_socket_get_ip_and_port(sg->nic[i].ip_address, nic_addr, OSAL_IP_BIN_ADDR_SZ,
                    &tmp_port_nr, &nic_is_ipv6, flags, IOC_DEFAULT_SOCKET_PORT);
                if (s) continue;

                if (opt_is_ipv6)
                {
                    if (!nic_is_ipv6) continue;

                    interface_ix = osal_get_interface_index_by_ipv6_address(iface_list_str, nic_addr);
                    if (interface_ix <  0) continue;

                    mreq6.ipv6mr_interface = (ULONG)osal_str_to_int(ipbuf, OS_NULL);
                    if (setsockopt(handle, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char*)&mreq6, sizeof(mreq6)) < 0)
                    {
                        s = OSAL_STATUS_MULTICAST_GROUP_FAILED;
                        goto getout;
                    }
                }
                else
                {
                    if (nic_is_ipv6) continue;

                    os_memcpy(&mreq.imr_interface.s_addr, nic_addr, OSAL_IPV4_BIN_ADDR_SZ);
                    if (setsockopt(handle, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)mr, mr_sz) < 0)
                    {
                        s = OSAL_STATUS_MULTICAST_GROUP_FAILED;
                        goto getout;
                    }
                }
                has_iface_addr = OS_TRUE;
            }
        }

        /* If we still got no interface addess, ask Windows for list of all useful interfaces.
         */
        if (!has_iface_addr)
        {
            /* We have done this already for IPv6. For IPv4 we need to look up adapters here.
             */
            if (interface_list == OS_NULL)
            {
                interface_list = osal_stream_buffer_open(OS_NULL, OS_NULL, OS_NULL, OSAL_STREAM_DEFAULT);
                osal_socket_list_network_interfaces(interface_list, af, OS_FALSE);
                iface_list_str = osal_stream_buffer_content(interface_list, OS_NULL);
            }
            p = iface_list_str;
            while (p)
            {
                e = os_strchr(p, ',');
                if (e == OS_NULL) e = os_strchr(p, '\0');
                if (e > p)
                {
                    n = (os_int)(e - p + 1);
                    if (n > sizeof(ipbuf)) n = sizeof(ipbuf);
                    os_strncpy(ipbuf, p, n);
                    if (opt_is_ipv6)
                    {
                        mreq6.ipv6mr_interface = (ULONG)osal_str_to_int(ipbuf, OS_NULL);
                        if (setsockopt(handle, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (char*)&mreq6, sizeof(mreq6)) < 0)
                        {
                            s = OSAL_STATUS_MULTICAST_GROUP_FAILED;
                            goto getout;
                        }
                    }
                    else
                    {
                        if (inet_pton(AF_INET, ipbuf, &mreq.imr_interface.s_addr) != 1) {
                            osal_debug_error_str("osal_socket_open: inet_pton() failed:", ipbuf);
                        }
                        if (setsockopt(handle, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) < 0)
                        {
                            s = OSAL_STATUS_MULTICAST_GROUP_FAILED;
                            goto getout;
                        }
                    }
                    has_iface_addr = OS_TRUE;
                }
                if (*e == '\0') break;
                p = e + 1;
            }
        }

        if (!has_iface_addr)
        {
            osal_error(OSAL_ERROR, eosal_mod, OSAL_STATUS_FAILED, "No interface addr");
            goto getout;
        }
    }

    /* Send for multicasts.
     */
    else
    {
        /* We need interface list to convert adapter addressess to adapter indices
         */
        if (af == AF_INET6)
        {
            interface_list = osal_stream_buffer_open(OS_NULL, OS_NULL, OS_NULL, OSAL_STREAM_DEFAULT);
            n_ifaces = osal_socket_list_network_interfaces(interface_list, af, OS_TRUE);
            iface_list_str = osal_stream_buffer_content(interface_list, OS_NULL);
        }

        if (has_iface_addr)
        {
            if (opt_is_ipv6)
            {
                interface_ix = osal_get_interface_index_by_ipv6_address(iface_list_str, iface_addr_bin);
                if (interface_ix >= 0)
                {
                    if (osal_socket_alloc_send_mcast_ifaces(mysocket, 1)) goto getout;
                    *(os_int*)mysocket->send_mcast_ifaces = interface_ix;
                }
                else
                {
                    has_iface_addr = OS_FALSE;
                    osal_debug_error("osal_setup_socket_for_u...: Multicast target iface not found");
                }
            }
            else
            {
                if (osal_socket_alloc_send_mcast_ifaces(mysocket, 1)) goto getout;
                os_memcpy(mysocket->send_mcast_ifaces, iface_addr_bin, OSAL_IPV4_BIN_ADDR_SZ);
            }
        }

        /* Address not a function parameter, see if we have it for the NIC
         */
        if (!has_iface_addr && (flags & OSAL_STREAM_USE_GLOBAL_SETTINGS))
        {
            if (osal_socket_alloc_send_mcast_ifaces(mysocket, sg->n_nics)) goto getout;
            ni = 0;
            for (i = 0; i < sg->n_nics; i++)
            {
                if (!sg->nic[i].send_udp_multicasts) continue;

                s = osal_socket_get_ip_and_port(sg->nic[i].ip_address, nic_addr, OSAL_IP_BIN_ADDR_SZ,
                    &tmp_port_nr, &nic_is_ipv6, flags, IOC_DEFAULT_SOCKET_PORT);
                if (s) continue;

                if (opt_is_ipv6)
                {
                    if (!nic_is_ipv6) continue;

                    interface_ix = osal_get_interface_index_by_ipv6_address(iface_list_str, nic_addr);
                    if (interface_ix <  0) continue;
                    ((os_int*)mysocket->send_mcast_ifaces)[ni] = interface_ix;
                }
                else
                {
                    if (nic_is_ipv6) continue;
                    os_memcpy(mysocket->send_mcast_ifaces + ni * OSAL_IPV4_BIN_ADDR_SZ,
                        nic_addr, OSAL_IPV4_BIN_ADDR_SZ);
                }
                ni++;
                has_iface_addr = OS_TRUE;
            }
            mysocket->send_mcast_ifaces_n = ni;
        }

        /* If we still got no interface addess, ask Windows for list of all useful interfaces.
         */
        if (!has_iface_addr)
        {
            /* We have done this already for IPv6. For IPv4 we need to look up adapters here.
             */
            if (interface_list == OS_NULL)
            {
                interface_list = osal_stream_buffer_open(OS_NULL, OS_NULL, OS_NULL, OSAL_STREAM_DEFAULT);
                n_ifaces = osal_socket_list_network_interfaces(interface_list, af, OS_FALSE);
                iface_list_str = osal_stream_buffer_content(interface_list, OS_NULL);
            }
            if (osal_socket_alloc_send_mcast_ifaces(mysocket, n_ifaces)) goto getout;

            ni = 0;
            p = iface_list_str;
            while (p)
            {
                e = os_strchr(p, ',');
                if (e == OS_NULL) e = os_strchr(p, '\0');
                if (e > p)
                {
                    n = (os_int)(e - p + 1);
                    if (n > sizeof(ipbuf)) n = sizeof(ipbuf);
                    os_strncpy(ipbuf, p, n);
                    if (opt_is_ipv6)
                    {
                        interface_ix = (os_int)osal_str_to_int(ipbuf, OS_NULL);
                        ((os_int*)mysocket->send_mcast_ifaces)[ni] = interface_ix;
                    }
                    else
                    {
                        if (inet_pton(AF_INET, ipbuf, nic_addr) != 1) {
                            osal_debug_error_str("osal_socket_open: inet_pton() failed:", ipbuf);
                        }
                        os_memcpy(mysocket->send_mcast_ifaces + ni * OSAL_IPV4_BIN_ADDR_SZ,
                            nic_addr, OSAL_IPV4_BIN_ADDR_SZ);
                    }
                    ni++;
                    has_iface_addr = OS_TRUE;
                }
                if (*e == '\0') break;
                p = e + 1;
            }
        }
    }

    /* We are good, cleanup, save socket handle and return.
     */
    osal_stream_close(interface_list, OSAL_STREAM_DEFAULT);
    mysocket->handle = handle;
    return OSAL_SUCCESS;

getout:
    /* Cleanup and return status code
     */
    if (handle != INVALID_SOCKET)
    {
        closesocket(handle);
    }
    osal_stream_close(interface_list, OSAL_STREAM_DEFAULT);
    return s;
}


/**
****************************************************************************************************

  @brief Allocate interface list (internal).
  @anchor osal_socket_alloc_send_mcast_ifaces

  Allocate empty list of of interfaces (either interface indexes for IPv6 or interface
  addressess for IPv4) where to send udp multicast. If n is 0, the list is released.

  @param   mysocket My socket structure.
           Number of interfaces.
  @return  OSAL_SUCCESS if all fine.

****************************************************************************************************
*/
static osalStatus osal_socket_alloc_send_mcast_ifaces(
    osalSocket *mysocket,
    os_int n)
{
    os_int sz;

    if (mysocket->send_mcast_ifaces)
    {
        os_free(mysocket->send_mcast_ifaces, mysocket->send_mcast_ifaces_sz);
        mysocket->send_mcast_ifaces = OS_NULL;
    }

    mysocket->send_mcast_ifaces_n = n;
    sz = n * (mysocket->is_ipv6 ? sizeof(os_int) : OSAL_IPV4_BIN_ADDR_SZ);
    mysocket->send_mcast_ifaces_sz = sz;
    if (n)
    {
        mysocket->send_mcast_ifaces = os_malloc(sz, OS_NULL);
        if (mysocket->send_mcast_ifaces == OS_NULL) return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        os_memclear(mysocket->send_mcast_ifaces, sz);
    }

    return OSAL_SUCCESS;
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

    /* If this is not multicast or listening socket.
     */
    if (mysocket->passive_port == 0)
    {
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
    }

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
    if (mysocket->open_flags & OSAL_STREAM_MULTICAST)
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

    /* Release memory allocated for multicast iface list.
     */
    osal_socket_alloc_send_mcast_ifaces(mysocket, 0);

    /* Free ring buffer, if any, and memory allocated for socket structure.
     */
    os_free(mysocket->buf, mysocket->buf_sz);
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
           The value OSAL_NO_NEW_CONNECTION indicates that no new incoming
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
    char addrbuf[INET6_ADDRSTRLEN];

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
            os_memclear(&sin_remote6, sizeof(sin_remote6));
            new_handle = accept(handle, (struct sockaddr*)&sin_remote6, &addr_size);
        }
        else
        {
            addr_size = sizeof(sin_remote);
            os_memclear(&sin_remote, sizeof(sin_remote));
            new_handle = accept(handle, (struct sockaddr*)&sin_remote, &addr_size);
        }

        /* If no new connection, do nothing more.
         */
        if (new_handle == INVALID_SOCKET)
        {
            if (status) *status = OSAL_NO_NEW_CONNECTION;
            return OS_NULL;
        }

        /* Set socket reuse, blocking mode
         */
        if (flags == OSAL_STREAM_DEFAULT) {
            flags = mysocket->open_flags;
        }
        if ((flags & OSAL_STREAM_NO_REUSEADDR) == 0) {
            if (setsockopt(new_handle, SOL_SOCKET,  SO_REUSEADDR,
                (char *)&on, sizeof(on)) < 0)
            {
                rval = OSAL_STATUS_FAILED;
                goto getout;
            }
        }
        if (ioctlsocket(new_handle, FIONBIO, &on) == SOCKET_ERROR) {
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
        newsocket->hdr.iface = &osal_socket_iface;

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

        if (remote_ip_addr)
        {
            if (mysocket->is_ipv6)
            {
                inet_ntop(AF_INET6, &sin_remote6.sin6_addr, addrbuf, sizeof(addrbuf));
                os_strncpy(remote_ip_addr, "[", remote_ip_addr_sz);
                os_strncat(remote_ip_addr, addrbuf, remote_ip_addr_sz);
                os_strncat(remote_ip_addr, "]", remote_ip_addr_sz);
            }
            else
            {
                inet_ntop(AF_INET, &sin_remote.sin_addr, addrbuf, sizeof(addrbuf));
                os_strncpy(remote_ip_addr, addrbuf, remote_ip_addr_sz);
            }
        }

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

  @brief Write data to socket (internal, no ring buffer).
  @anchor osal_socket_write2

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
    os_int rval, werr;
    SOCKET handle;
    osalStatus status = OSAL_SUCCESS;

    /* Ret operating system's socket handle and write to the socket.
    */
    handle = mysocket->handle;
    rval = send(handle, buf, (int)n, 0);

    if (rval == SOCKET_ERROR)
    {
        werr = WSAGetLastError();

        /* This matches with net_sockets.c
         */
        switch (werr)
        {
            case WSAEWOULDBLOCK:
            // case WSAENOTCONN: /* WSAENOTCONN = socket not (yet?) connected. */
                break;

            case WSAECONNREFUSED:
                status = OSAL_STATUS_CONNECTION_REFUSED;
                break;

            case WSAECONNRESET:
                status = OSAL_STATUS_CONNECTION_RESET;
                break;

            default:
                status = OSAL_STATUS_FAILED;
                break;
        }

        rval = 0;
    }

    *n_written = rval;
    return status;
}


/**
****************************************************************************************************

  @brief Write data to socket (trough ring buffer).
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

            /* This matches with net_sockets.c
             */
            switch (werr)
            {
                case WSAEWOULDBLOCK:
                case WSAENOTCONN: /* WSAENOTCONN = socket not (yet?) connected. */
                    break;

                case WSAECONNREFUSED:
                    status = OSAL_STATUS_CONNECTION_REFUSED;
                    goto getout;

                case WSAECONNRESET:
                    status = OSAL_STATUS_CONNECTION_RESET;
                    goto getout;

                default:
                    status = OSAL_STATUS_FAILED;
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
    os_int i, n_sockets, n_events, event_nr;
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
        selectdata->stream_nr = OSAL_STREAM_NR_TIMEOUT_EVENT;
        return OSAL_SUCCESS;
    }

    event_nr = (os_int)(rval - WSA_WAIT_EVENT_0);

    if (evnt && event_nr == n_sockets)
    {
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
    const os_char *buf,
    os_memsz n,
    os_int flags)
{
    osalSocket *mysocket;
    struct sockaddr_in sin_remote;
    struct sockaddr_in6 sin_remote6;
    int nbytes, werr;
    struct ip_mreq mreq;
    struct ipv6_mreq mreq6;
    os_int n_ifaces, i;
    osalStatus s = OSAL_SUCCESS;

    if (stream == OS_NULL) return OSAL_STATUS_FAILED;

    /* Cast stream pointer to socket structure pointer.
     */
    mysocket = (osalSocket*)stream;
    osal_debug_assert(mysocket->hdr.iface == &osal_socket_iface && mysocket->send_mcast_ifaces);
    n_ifaces = mysocket->send_mcast_ifaces_n;

    if (mysocket->is_ipv6)
    {
        /* Set up destination address
         */
        os_memclear(&sin_remote6, sizeof(sin_remote6));
        sin_remote6.sin6_family = AF_INET6;
        sin_remote6.sin6_port = htons(mysocket->passive_port);
        memcpy(&sin_remote6.sin6_addr, mysocket->multicast_group, OSAL_IPV6_BIN_ADDR_SZ);

        /* Loop trough interfaces to which to send thee multicast
         */
        for (i = 0; i < n_ifaces; i++)
        {
            /* Select network interface to use.
             */
            os_memclear(&mreq6, sizeof(mreq6));
            mreq6.ipv6mr_interface = ((os_int*)mysocket->send_mcast_ifaces)[i];

            if (setsockopt(mysocket->handle, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char*)&mreq6, sizeof(mreq6)) < 0)
            {
                osal_error(OSAL_ERROR, eosal_mod, OSAL_STATUS_SELECT_MULTICAST_IFACE_FAILED, OS_NULL);
                s = OSAL_STATUS_SELECT_MULTICAST_IFACE_FAILED;
                continue;
            }

            /* Send packet.
             */
            nbytes = sendto(mysocket->handle, buf, (int)n, 0,
                (struct sockaddr*)&sin_remote6, sizeof(sin_remote6));

            /* Handle "sendto" errors.
             */
            if (nbytes < 0)
            {
                werr = WSAGetLastError();
                switch (werr)
                {
                    case WSAEWOULDBLOCK:
                    case WSAENOTCONN: /* WSAENOTCONN = socket not (yet?) connected. */
                        if (!s) s = OSAL_PENDING;
                        break;

                    case WSAECONNREFUSED:
                        s = OSAL_STATUS_CONNECTION_REFUSED;
                        break;

                    case WSAECONNRESET:
                        s = OSAL_STATUS_CONNECTION_RESET;
                        break;

                    default:
                        s = OSAL_STATUS_SEND_MULTICAST_FAILED;
                        break;
                }
            }
        }
    }
    else
    {
        /* Set up destination address
         */
        os_memclear(&sin_remote, sizeof(sin_remote));
        sin_remote.sin_family = AF_INET;
        sin_remote.sin_port = htons(mysocket->passive_port);
        memcpy(&sin_remote.sin_addr.s_addr, mysocket->multicast_group, OSAL_IPV4_BIN_ADDR_SZ);

        /* Loop trough interfaces to which to send thee multicast
         */
        for (i = 0; i < n_ifaces; i++)
        {
            /* Select network interface to use.
             */
            os_memclear(&mreq, sizeof(mreq));
            os_memcpy(&mreq.imr_interface.s_addr, mysocket->send_mcast_ifaces
                + i * OSAL_IPV4_BIN_ADDR_SZ, OSAL_IPV4_BIN_ADDR_SZ);

            if (setsockopt(mysocket->handle, IPPROTO_IP, IP_MULTICAST_IF, (char*)&mreq, sizeof(mreq)) < 0)
            {
                osal_error(OSAL_ERROR, eosal_mod, OSAL_STATUS_SELECT_MULTICAST_IFACE_FAILED, OS_NULL);
                s = OSAL_STATUS_SELECT_MULTICAST_IFACE_FAILED;
                continue;
            }

            /* Send packet.
             */
            nbytes = sendto(mysocket->handle, buf, (int)n, 0,
                (struct sockaddr*)&sin_remote, sizeof(sin_remote));

            /* Handle "sendto" errors.
             */
            if (nbytes < 0)
            {
                werr = WSAGetLastError();
                switch (werr)
                {
                    case WSAEWOULDBLOCK:
                    case WSAENOTCONN: /* WSAENOTCONN = socket not (yet?) connected. */
                        if (!s) s = OSAL_PENDING;
                        break;

                    case WSAECONNREFUSED:
                        s = OSAL_STATUS_CONNECTION_REFUSED;
                        break;

                    case WSAECONNRESET:
                        s = OSAL_STATUS_CONNECTION_RESET;
                        break;

                    default:
                        s = OSAL_STATUS_SEND_MULTICAST_FAILED;
                        break;
                }
            }
        }
    }

    if (s)
    {
        osal_error(OSAL_ERROR, eosal_mod, OSAL_STATUS_SEND_MULTICAST_FAILED, OS_NULL);
    }

    return s;
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
    struct sockaddr_in6 sin_remote6;
    int nbytes, werr;
    socklen_t addr_size;
    char addrbuf[INET6_ADDRSTRLEN];
    osalStatus status;

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
        os_memclear(&sin_remote6, sizeof(sin_remote6));
        addr_size = sizeof(sin_remote6);
        nbytes = recvfrom(mysocket->handle, buf, (int)n, 0,
            (struct sockaddr*)&sin_remote6, &addr_size);
    }
    else
    {
        os_memclear(&sin_remote, sizeof(sin_remote));
        addr_size = sizeof(sin_remote);
        nbytes = recvfrom(mysocket->handle, buf, (int)n, 0,
            (struct sockaddr*)&sin_remote, &addr_size);
    }

    if (nbytes < 0)
    {
        werr = WSAGetLastError();
        switch (werr)
        {
            case WSAEWOULDBLOCK:
            case WSAENOTCONN: /* WSAENOTCONN = socket not (yet?) connected. */
                status = OSAL_PENDING;
                break;

            case WSAECONNREFUSED:
                status = OSAL_STATUS_CONNECTION_REFUSED;
                break;

            case WSAECONNRESET:
                status = OSAL_STATUS_CONNECTION_RESET;
                break;

            default:
                status = OSAL_STATUS_RECEIVE_MULTICAST_FAILED;
                break;
        }
        return status;
    }

    if (remote_addr)
    {
        if (mysocket->is_ipv6)
        {
            inet_ntop(AF_INET6, &sin_remote6.sin6_addr, addrbuf, sizeof(addrbuf));
            os_strncpy(remote_addr, "[", remote_addr_sz);
            os_strncat(remote_addr, addrbuf, remote_addr_sz);
            os_strncat(remote_addr, "]", remote_addr_sz);
        }
        else
        {
            inet_ntop(AF_INET, &sin_remote.sin_addr, addrbuf, sizeof(addrbuf));
            os_strncpy(remote_addr, addrbuf, remote_addr_sz);
        }
    }

    if (n_read) *n_read = nbytes;
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief List network interfaces which can be used for UDP multicasts.
  @anchor osal_socket_list_network_interfaces

  The osal_socket_list_network_interfaces() function ...

  It is stuck in there very deep.
  The member you want is FirstUnicastAddress, this has a member Address which is of type
  SOCKET_ADDRESS, which has a member named lpSockaddr which is a pointer to a SOCKADDR structure.
  Once you get to this point you should notice the familiar winsock structure and be able to
  get the address on your own.

  @param   interface_list Stream into which write the interface list. In practice stream
           buffer to simply to hold variable length string.
           For example for IPv4 "192.168.1.229,192.168.80.1,192.168.10.1,169.254.102.98"
  @param   family Address family AF_INET or AF_INET6.
  @param   get_interface_index If OS_TRUE the function returns list of interface indiexes in
           addition to IP addresses. Format will be like
           "4=2600:1700:20c0:7050::35,22=fe80::ac67:637f:82a3:f4ae,10=fe80::c9a7:1924:8b0d:3d5f".
           This option is needed only with AF_INET6, when we need adapter indexes, but is
           implemented also for IPv4.

  @return  Number of interfaces, or 0 if failed.

****************************************************************************************************
*/
static os_int osal_socket_list_network_interfaces(
    osalStream interface_list,
    os_uint family,
    os_boolean get_interface_index)
{
/* Link with Iphlpapi.lib */
#pragma comment(lib, "IPHLPAPI.lib")

    char buf[OSAL_IPADDR_SZ];
    os_memsz n_written;
    os_memsz outbuf_sz;
    ULONG win_outbuf_sz;
    os_boolean n_interfaces;
    const os_int max_tries = 3;
    os_int i;

    ULONG flags;
    LPVOID lpMsgBuf = NULL;
    DWORD rval;
    PIP_ADAPTER_ADDRESSES pAddresses;
    PIP_ADAPTER_ADDRESSES pCurrAddresses;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast;
    struct sockaddr_in *sa_in;
    struct sockaddr_in6 *sa_in6;

    /* Allocate a 15 KB buffer to start with.
     */
    outbuf_sz = 15000;

    /* Set the flags to pass to GetAdaptersAddresses
     */
    flags =
          GAA_FLAG_SKIP_FRIENDLY_NAME |
          GAA_FLAG_SKIP_ANYCAST |
          GAA_FLAG_SKIP_MULTICAST |
          GAA_FLAG_SKIP_DNS_SERVER;

    i = 0;
    n_interfaces = 0;
    do {
        pAddresses = (IP_ADAPTER_ADDRESSES *) os_malloc(outbuf_sz, &outbuf_sz);
        if (pAddresses == NULL) {
            goto getout;
        }

        win_outbuf_sz = (ULONG)outbuf_sz;
        rval = GetAdaptersAddresses(family, flags, NULL, pAddresses, &win_outbuf_sz);
        if (rval == ERROR_BUFFER_OVERFLOW) {
            os_free(pAddresses, outbuf_sz);
            pAddresses = NULL;
            outbuf_sz = (os_memsz)win_outbuf_sz;
        }
        else {
            break;
        }
    }
    while ((rval == ERROR_BUFFER_OVERFLOW) && (++i < max_tries));

    if (rval == NO_ERROR)
    {
        pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            /* Skip if no multicast (we are looking for it). Filter also for other reasons.
               What should be done if pCurrAddresses->OperStatus is IfOperStatusDormant?
             */
            if (pCurrAddresses->NoMulticast) goto goon;
            if (family == AF_INET) if (!pCurrAddresses->Ipv4Enabled) goto goon;
            if (family == AF_INET6) if (!pCurrAddresses->Ipv6Enabled) goto goon;
            if (pCurrAddresses->IfType != IF_TYPE_IEEE80211 &&
                pCurrAddresses->IfType != IF_TYPE_ETHERNET_CSMACD &&
                pCurrAddresses->IfType != IF_TYPE_SOFTWARE_LOOPBACK) goto goon;
            if (pCurrAddresses->OperStatus != IfOperStatusUp) goto goon;

            pUnicast = pCurrAddresses->FirstUnicastAddress;
            if (pUnicast != NULL)
            {
                /* Uh huh, it seems to be here. We only need first unicast address, otherwise
                   we could loop with pUnicast = pUnicast->Next;
                 */
                SOCKADDR *sa = pUnicast->Address.lpSockaddr;
                if (sa)
                {
                    if (sa->sa_family == AF_INET)
                    {
                        if (n_interfaces++) {
                            osal_stream_print_str(interface_list, ",", 0);
                        }
                        if (get_interface_index) {
                            osal_int_to_str(buf, sizeof(buf), pCurrAddresses->IfIndex);
                            osal_stream_print_str(interface_list, buf, 0);
                            osal_stream_print_str(interface_list, "=", 0);
                        }
                        sa_in = (struct sockaddr_in *)sa;
                        inet_ntop(AF_INET,&(sa_in->sin_addr),buf,sizeof(buf));
                        osal_stream_print_str(interface_list, buf, 0);
                    }
                    else if (sa->sa_family == AF_INET6)
                    {
                        if (n_interfaces++) {
                            osal_stream_print_str(interface_list, ",", 0);
                        }
                        if (get_interface_index) {
                            osal_int_to_str(buf, sizeof(buf), pCurrAddresses->Ipv6IfIndex);
                            osal_stream_print_str(interface_list, buf, 0);
                            osal_stream_print_str(interface_list, "=", 0);
                        }
                        sa_in6 = (struct sockaddr_in6 *)sa;
                        inet_ntop(AF_INET6,&(sa_in6->sin6_addr),buf,sizeof(buf));
                        osal_stream_print_str(interface_list, buf, 0);
                    }
                }
            }

goon:
            pCurrAddresses = pCurrAddresses->Next;
        }
    }

    /* Something went wrong with Windows, generate debug info.
     */
#if OSAL_DEBUG
    else
    {
        if (rval == ERROR_NO_DATA) {
            osal_debug_error("GetAdaptersAddresses returned no data?");
        }
        else {
            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, rval, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &lpMsgBuf, 0, NULL))
            {
                osal_debug_error_str("GetAdaptersAddresses failed: ", (os_char*)lpMsgBuf);
                LocalFree(lpMsgBuf);
            }
        }
    }
#endif

    /* Almost done, free buffer, terminate interface list with NULL character and return status.
     */
    os_free(pAddresses, outbuf_sz);
getout:
    osal_stream_write(interface_list, osal_str_empty, 1, &n_written, OSAL_STREAM_DEFAULT);
    return n_interfaces;
}


/**
****************************************************************************************************

  @brief Find network interface index by IP address.
  @anchor osal_get_interface_index_by_ipv6_address

  The osal_get_interface_index_by_ipv6_address() function searches for network interface
  list to find interface index for a network adapter. This is needed because we select
  to which adapter we send an UDP multicast by interface address (inherited from IPv4)
  and IPv6 multicast functions require adapter index.

  @param   iface_list_str Pointer to interface list string, Format like
           "4=2600:1700:20c0:7050::35,22=fe80::ac67:637f:82a3:f4ae,10=fe80::c9a7:1924:8b0d:3d5f".
  @param   iface_addr_bin IPv6 address, 16 bytes.

  @return  Interface index, -1 if none found.

****************************************************************************************************
*/
static os_int osal_get_interface_index_by_ipv6_address(
    os_char *iface_list_str,
    os_char *iface_addr_bin)
{
    os_char ipbuf[OSAL_IPADDR_SZ], *p, *e, *q, addr[OSAL_IP_BIN_ADDR_SZ];
    os_int i, n, interface_ix;

    p = iface_list_str;
    while (p)
    {
        e = os_strchr(p, ',');
        if (e == OS_NULL) e = os_strchr(p, '\0');
        if (e > p)
        {
            n = (os_int)(e - p + 1);
            if (n > sizeof(ipbuf)) n = sizeof(ipbuf);
            os_strncpy(ipbuf, p, n);

            interface_ix = (os_int)osal_str_to_int(ipbuf, OS_NULL);
            q = os_strchr(ipbuf, '=');
            if (q == OS_NULL) return -1;

            if (inet_pton(AF_INET6, q + 1, addr) != 1) {
                osal_debug_error_str("osal_get_interface_index_by_ipv6_address: inet_pton() failed:", ipbuf);
            }
            else
            {
                for (i = 0; i < OSAL_IPV6_BIN_ADDR_SZ; i++) {
                    if (iface_addr_bin[i] != addr[i]) break;
                }
                if (i == OSAL_IPV6_BIN_ADDR_SZ)
                {
                    return interface_ix;
                }
            }
        }
        if (*e == '\0') break;
        p = e + 1;
    }

    return -1;
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


/**
****************************************************************************************************

  @brief Set up a ring buffer.
  @anchor osal_socket_setup_ring_buffer

  The osal_socket_setup_ring_buffer() function...

  @param  socket Pointer to our socket structure for Windows.
  @return None.

****************************************************************************************************
*/
static void osal_socket_setup_ring_buffer(
    osalSocket *mysocket)
{
    mysocket->buf_sz = 1420; /* selected for TCP sockets */
    mysocket->buf = os_malloc(mysocket->buf_sz, OS_NULL);
}


OS_FLASH_MEM osalStreamInterface osal_socket_iface
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
