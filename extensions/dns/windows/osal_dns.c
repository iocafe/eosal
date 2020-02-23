/**

  @file    dns/windows/osal_dns.c
  @brief   Resolve host name or IP address string for Windows sockets.
  @author  Pekka Lehtikoski
  @version 1.1
  @date    23.2.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_SOCKET_SUPPORT

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

static osalStatus osal_gethostbyname_sys(
    const os_char *name,
    os_char *addr,
    os_memsz addr_sz,
    os_boolean *is_ipv6);

/**
****************************************************************************************************

  @brief Get computer's binary address by name or IP address string.
  @anchor osal_gethostbyname

  The osal_gethostbyname gets binary IP address of a by computer name or IP address string.
  Here name is either a hostname, or an IPv4 address in standard dot notation, or an
  IPv6 address in colon (and possibly dot) notation.

  If name is empty string: If listening, listen all IP addressess. If connecting, use local host.

  If name is looked up from DNS and has both IPv4 and IPv6 addressess, IPv4 address is selected.

  @param   name Computer name or IP address string.
  @param   addr Pointer where to store the binary IP address. IP address is stored in
           network byte order (most significant byte first). Either 4 or 16 bytes are stored
           depending if this is IPv4 or IPv6 address. Entire buffer is anythow cleared.
  @param   addr_sz Address buffer size in bytes. This should be minimum 16 bytes to allow
           storing IPv6 address.
  @param   is_ipv6 Pointer to boolean to set to OS_TRUE if this is IPv6 address or OE_FALSE
           if this is IPv4 address.
  @param   default_use_flags What socket is used for. This is used to make defaule IP address
           if it is omitted from parameters" string. Set either OSAL_STREAM_CONNECT (0) or
           OSAL_STREAM_LISTEN (0x0100) depending which end of the socket we are preparing.
           Bit fields, can be stream flags directly.

  @return  If IP address is successfully retrieved, the function returns OSAL_SUCCESS. Other
           return values indicate that hostname didn't match any known host, or an error occurred.

****************************************************************************************************
*/
osalStatus osal_gethostbyname(
    const os_char *name,
    os_char *addr,
    os_memsz addr_sz,
    os_boolean *is_ipv6,
    os_int default_use_flags)
{
    os_uint uaddr;
    osalStatus s;

    os_memclear(addr, addr_sz);
    *is_ipv6 = OS_FALSE;
    s = OSAL_STATUS_FAILED;

    /* Nowdays we enforce allocating enough memory also for IPv6.
     */
    if (addr_sz < 16)
    {
        goto getout;
    }

    /* If address is empty: If listening, listen all IP addressess. If connecting,
       use local host.
     */
    if (*name == '\0')
    {
        if (default_use_flags & OSAL_STREAM_LISTEN)
        {
            uaddr = htonl(INADDR_ANY);
            os_memcpy(addr, &uaddr, sizeof(uaddr));
            return OSAL_SUCCESS;
        }
        else
        {
            name = "127.0.0.1";
        }
    }

    s = osal_gethostbyname_sys(name, addr, addr_sz, is_ipv6);

getout:
    return s;
}


/**
****************************************************************************************************

  @brief Get computer's binary address by name or IP address string (internal).
  @anchor osal_gethostbyname_sys

  The osal_gethostbyname_sys gets binary IP address of a by computer name or IP address string.
  Here name is either a hostname, or an IPv4 address in standard dot notation, or an
  IPv6 address in colon (and possibly dot) notation.

  If name is empty string: If listening, listen all IP addressess. If connecting, use local host.

  If name is looked up from DNS and has both IPv4 and IPv6 addressess, IPv4 address is selected.

  @param   name Computer name or IP address string.
  @param   addr Pointer where to store the binary IP address. IP address is stored in
           network byte order (most significant byte first). Either 4 or 16 bytes are stored
           depending if this is IPv4 or IPv6 address. Entire buffer is anythow cleared.
  @param   addr_sz Address buffer size in bytes. This should be minimum 16 bytes to allow
           storing IPv6 address.
  @param   is_ipv6 Pointer to boolean to set to OS_TRUE if this is IPv6 address or OE_FALSE
           if this is IPv4 address.
  @param   default_use_flags What socket is used for. This is used to make defaule IP address
           if it is omitted from parameters" string. Set either OSAL_STREAM_CONNECT (0) or
           OSAL_STREAM_LISTEN (0x0100) depending which end of the socket we are preparing.
           Bit fields, can be stream flags directly.

  @return  If IP address is successfully retrieved, the function returns OSAL_SUCCESS. Other
           return values indicate that hostname didn't match any known host, or an error occurred.

****************************************************************************************************
*/
static osalStatus osal_gethostbyname_sys(
    const os_char *name,
    os_char *addr,
    os_memsz addr_sz,
    os_boolean *is_ipv6)
{
    struct addrinfo hints, *res, *p;
    struct sockaddr_in6 *ipv6;
    struct sockaddr_in *ipv4;
    os_char buf[128];
    int status;
    osalStatus s = OSAL_STATUS_FAILED;
 
    os_memclear(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
 
    if ((status = getaddrinfo(name, NULL, &hints, &res)) != 0) {
        osal_str_utf16_to_utf8(buf, sizeof(buf), gai_strerror(status));
        osal_debug_error_str("getaddrinfo: ", buf);
        return OSAL_STATUS_FAILED;
    }

    /* For now we prefer IPv4 address if we have both, so we check for these first.
     */
    for (p = res; p != NULL; p = p->ai_next) 
    {
        if (p->ai_family == AF_INET) 
        {
            ipv4 = (struct sockaddr_in *)p->ai_addr;
            if (addr_sz >= OSAL_IPV4_BIN_ADDR_SZ)
            {
                os_memcpy(addr, &(ipv4->sin_addr), OSAL_IPV4_BIN_ADDR_SZ);
                s = OSAL_SUCCESS;
            }
            goto getout;
        }
    }

    /* Get IPv6 address if we didn't get IPv4 address.
     */
    for (p = res; p != NULL; p = p->ai_next) 
    {
        if (p->ai_family == AF_INET6) 
        {
            if (addr_sz >= OSAL_IPV6_BIN_ADDR_SZ)
            {
                ipv6 = (struct sockaddr_in6 *)p->ai_addr;

                os_memcpy(addr, &(ipv6->sin6_addr), OSAL_IPV6_BIN_ADDR_SZ);
                *is_ipv6 = OS_TRUE;
                s = OSAL_SUCCESS;
            }
            goto getout;
        }
    }
 
getout:
    freeaddrinfo(res);
    return s;
}

#endif
