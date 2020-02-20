/**

  @file    dns/arduino/osal_dns.c
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
#if OSAL_SOCKET_SUPPORT

/**
****************************************************************************************************

  @brief Get computer's binary address by name or IP address string.
  @anchor osal_gethostbyname

  NOT SUPPORTED FOR ARDUINO FOR NOW, JUST CONVERTS NUMERIC IP

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
    os_memclear(addr, addr_sz);
    *is_ipv6 = OS_FALSE;
    return osal_ip_from_str((os_uchar*)addr, addr_sz, name);
}

#endif