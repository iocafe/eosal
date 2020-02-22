/**

  @file    dns/windows/osal_dns.c
  @brief   Resolve host name or IP address string for Windows sockets.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    1.2.2020

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
// #include <stdio.h>
#include <windows.h>

/**
****************************************************************************************************

  @brief Get computer's binary address by name or IP address string.
  @anchor osal_gethostbyname

  The osal_gethostbyname gets binary IP address of a by computer name or IP address string.
  Here name is either a hostname, or an IPv4 address in standard dot notation, or an
  IPv6 address in colon (and possibly dot) notation.

  If name is empty string: If listening, listen all IP addressess. If connecting, use local host.

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
    struct hostent *hp;
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

    hp = gethostbyname(name);
    if (hp == NULL) return OSAL_STATUS_FAILED;

    /* Copy the host address. Recall that the host might be connected to multiple networks
       and have different addresses on each one.) The vector is terminated by a null pointer.
       At least for now, we use always the first address.
     */
    os_memcpy(addr, hp->h_addr_list[0], hp->h_length);
    *is_ipv6 = (os_boolean)(hp->h_addrtype == AF_INET6);
    s = OSAL_SUCCESS;

getout:
    return s;
}

#if 0
We should use getaddrinfo to add support for IPv6, in case IPv4 not found

int OpenConnection(const char *hostname, const char *port)
{
    struct hostent *host;
    if ((host = gethostbyname(hostname)) == nullptr)
    {
        //More descriptive error message?
        perror(hostname);
        exit(EXIT_FAILURE);
    }

    struct addrinfo hints = {0}, *addrs;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    const int status = getaddrinfo(hostname, port, &hints, &addrs);
    if (status != 0)
    {
        fprintf(stderr, "%s: %s\n", hostname, gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    int sfd, err;
    for (struct addrinfo *addr = addrs; addr != nullptr; addr = addr->ai_next)
    {
        sfd = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
        if (sfd == ERROR_STATUS)
        {
            err = errno;
            continue;
        }

        if (connect(sfd, addr->ai_addr, addr->ai_addrlen) == 0)
        {
            break;
        }

        err = errno;
        sfd = ERROR_STATUS;
        close(sfd);
    }

    freeaddrinfo(addrs);

    if (sfd == ERROR_STATUS)
    {
        fprintf(stderr, "%s: %s\n", hostname, strerror(err));
        exit(EXIT_FAILURE);
    }

    return sfd;
}
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
 
int main(int argc, char *argv[])
{
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];
 
    if (argc != 2) {
fprintf(stderr, "Usage: %s hostname\n", argv[0]);
return 1;
}
 
memset(&hints, 0, sizeof hints);
hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
hints.ai_socktype = SOCK_STREAM;
 
if ((status = getaddrinfo(argv[1], NULL, &hints, &res)) != 0) {
fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
return 2;
}
 
for(p = res;p != NULL; p = p->ai_next) {
 
void *addr;
 
if (p->ai_family == AF_INET) {
return 1;
 
} else {
 
struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
addr = &(ipv6->sin6_addr);
 
/* convert the IP to a string and print it: */
inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
 
printf("Hostname: %s\n", argv[1]);
printf("IP Address: %s\n", ipstr);
 
}
}
 
freeaddrinfo(res); // free the linked list
 
return 0;
}
#endif



#endif
