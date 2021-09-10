/**

  @file    net/duino/osal_duino/dns.c
  @brief   Resolve host name or IP address string for arduino sockets.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    28.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#ifdef OSAL_ARDUINO
#if OSAL_SOCKET_SUPPORT
#if (OSAL_SOCKET_SUPPORT & OSAL_SOCKET_MASK) == OSAL_ARDUINO_ETHERNET_AP || \
    (OSAL_SOCKET_SUPPORT & OSAL_SOCKET_MASK) == OSAL_ARDUINO_WIFI_API || \
    (OSAL_SOCKET_SUPPORT & OSAL_SOCKET_MASK) == OSAL_SAM_WIFI_API


/**
****************************************************************************************************

  @brief Get computer's binary address by name or IP address string.
  @anchor osal_gethostbyname

  NOT SUPPORTED FOR ARDUINO FOR NOW, JUST CONVERTS NUMERIC IP

  The osal_gethostbyname gets binary IP address of a by computer name or IP address string.
  Here name is either a hostname, or an IPv4 address in standard dot notation, or an
  IPv6 address using colon separator.

  If name is empty string: If listening, listen all IP addressess. If connecting, use local host.

  @param   name Computer name or IP address string.
  @param   addr Pointer where to store the binary IP address. IP address is stored in
           network byte order (most significant byte first). Either 4 or 16 bytes are stored
           depending if this is IPv4 or IPv6 address. Unused bytes in buffer are zeroed.
  @param   addr_sz Address buffer size in bytes. This should be minimum 16 bytes to allow
           storing IPv6 address.
  @param   is_ipv6 Pointer to boolean. At input this is hint wether the caller prefers IPv4
           or IPv6 address. Host names may have both IPv4 and IPv6 address assigned.
           The function sets this to OS_TRUE if IPv6 address is selected, or to OS_FALSE for
           IPv4 address.
  @param   default_use_flags What socket is used for. This is used to make the default IP address
           if it is omitted from parameters" string. Set either OSAL_STREAM_CONNECT (0) or
           OSAL_STREAM_LISTEN depending which end of the socket we are preparing.
           OSAL_STREAM_MULTICAST if we are using the address for multicasts.
           Bit fields, can be stream flags as they are, extra flags are ignored.

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
    os_memclear(addr, addr_sz);
    *is_ipv6 = OS_FALSE;
    return osal_ip_from_str((os_uchar*)addr, addr_sz, name);
}

#endif
#endif
#endif