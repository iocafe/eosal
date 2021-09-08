/**

  @file    socket/common/osal_socket_util.c
  @brief   Socket helper functions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Socket helper functions common to all operating systems.

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

  @brief Get host and port from network address string.
  @anchor osal_socket_get_ip_and_port

  The osal_socket_get_ip_and_port() function converts the network address string used by
  eosal library to binary IP address and port number.

  @param   parameters Socket parameters, a parameter string like  "host:port". IPv6 host
           name should be within square brackets, like "[host]:port". "Host" string can
           be either host name or IP address. Asterix '*' as host name, or empty host name
           mean default address (default_use_flags). Asterix '*' or empty port number
           means default port. Marking like ":122" or "122" can be used just to specify port
           number to listen to.
  @param   addr Pointer where to store the binary IP address. IP address is stored in
           network byte order (most significant byte first). Either 4 or 16 bytes are stored
           depending if this is IPv4 or IPv6 address. Entire buffer is anythow cleared.
           Addr can be OS_NULL if not needed (makes the function OS independent).
           THis can be also address, string, if addr_sz is negative.
  @param   addr_sz Address buffer size in bytes. This should be minimum 16 bytes to allow
           storing IPv6 address. Negative addr_size indicates that we want address string
           back.
  @param   is_ipv6 Pointer to boolean to set to OS_TRUE if this is IPv6 address or OS_FALSE
           if this is IPv4 address.
  @param   port_nr Pointer to integer into which to store the port number.
  @param   is_ipv6 Flag to set if IP v6 address has been selected.
  @param   default_use_flags What socket is used for. This is used to make the default IP address
           if it is omitted from parameters" string. Set either OSAL_STREAM_CONNECT (0) or
           OSAL_STREAM_LISTEN depending which end of the socket we are preparing.
           OSAL_STREAM_MULTICAST if we are using the address for multicasts.
           Bit fields, can be stream flags as they are, extra flags are ignored.
  @param   default_port_nr Default port number to return if port number is not specified
           in parameters string.

  @return  If IP address is successfully retrieved, the function returns OSAL_SUCCESS. Other
           return values indicate that hostname didn't match any known host, or an error occurred.

****************************************************************************************************
*/
osalStatus osal_socket_get_ip_and_port(
    const os_char *parameters,
    os_char *addr,
    os_memsz addr_sz,
    os_int  *port_nr,
    os_boolean *is_ipv6,
    os_int default_use_flags,
    os_int default_port_nr)
{
    os_char buf[OSAL_HOST_BUF_SZ];
    os_char *port_pos, *addr_pos;

    os_strncpy(buf, parameters, sizeof(buf));

    /* Terminate address with '\0' character, search for port number position within the string.
     */
    port_pos = os_strchr(buf, ']');
    addr_pos = buf;
    if (port_pos)
    {
        *(port_pos++) = '\0';
        if (*(port_pos++) != ':') port_pos = (os_char*)osal_str_empty;
    }
    else
    {
        port_pos = os_strchr(buf, ':');
        if (port_pos) {
            *(port_pos++) = '\0';
        }
        else {
            if (os_strchr(buf, '.') == OS_NULL && osal_char_isdigit(buf[0])) {
                port_pos = buf;
                addr_pos = (os_char*)osal_str_empty;
            }
            else {
                port_pos = (os_char*)osal_str_empty;
            }
        }
    }

    /* Parse port number from string. Asterix '*' and empty port number in port number is selects
       default port.
     */
    if (*port_pos != '\0' && os_strchr(port_pos, '*') == OS_NULL)
    {
        *port_nr = (os_int)osal_str_to_int(port_pos, OS_NULL);
    }
    else
    {
        *port_nr = default_port_nr;
    }

    /* Make hint for osal_gethostbyname. IPv6 address is proposed if address is within square
       brackets, otherwise default to IPv4 address.
     */
    *is_ipv6 = addr_pos[0] == '[';

    /* Asterix '*' as host name is same as empty host name. We will still proceed to call
       osal_gethostbyname to get specific operating system specific addr settings.
     */
    if (os_strchr(addr_pos, '*')) {
        addr_pos = (os_char*)osal_str_empty;
    }

    /* If caller doesn't want IP address.
     */
    if (addr == OS_NULL) {
        return OSAL_SUCCESS;
    }

    /* Negative address srting size means that caller wants address string, not the
     * binary address.
     */
    if (addr_sz < 0) {
        os_strncpy(addr, addr_pos, -addr_sz);
        return OSAL_SUCCESS;
    }

    /* Convert to binary IP. If starts with bracket, skip it. This is operating system specific.
     */
    return osal_gethostbyname(addr_pos[0] == '[' ? addr_pos + 1 : addr_pos, addr, addr_sz,
        is_ipv6, default_use_flags);
}


/**
****************************************************************************************************

  @brief If port number is not specified in "parameters" string, then embed defaut port number.
  @anchor osal_socket_embed_default_port

  The osal_socket_embed_default_port() function examines the parameters string. If the parameter
  string already has TCP port number, the parameter string is copied to buffer as is.
  If not, modified parameter string which includes the port number is stored in buf.

  @param   parameters Socket parameters, a list string. "addr=host:port" or simply
           parameter string starting with "host:port", set host name or numeric IP address
           and port number. Host may be in the brackets, like "[host]:port". This is mostly used
           for IP V6 addresses, which themselves may contain colons ':'.
           Marking like ":122" can be used just to specify port number to listen to.
  @param   buf Pointer to buffer where to store modified parameter string. Buffer of
           OSAL_HOST_BUF_SZ bytes is recommended.
  @param   buf_sz Size of the buffer.
  @param   default_port_nr Port number to embed in parameters string, if port
           number is not already specified.

  @return  None.

****************************************************************************************************
*/
void osal_socket_embed_default_port(
    const os_char *parameters,
    os_char *buf,
    os_memsz buf_sz,
    os_int default_port_nr)
{
    os_char *p, nbuf[OSAL_NBUF_SZ];

    /* If we already have the port, leave as is.
     */
    os_strncpy(buf, parameters, buf_sz);
    p = os_strchr(buf, ']');
    if (p == OS_NULL) p = buf;
    p = os_strchr(p, ':');
    if (p) return;

    /* If this is only port number, change to ":port".
     */
    for (p = buf; *p != '\0'; p++) {
        if (!osal_char_isdigit(*p)) break;
    }
    if (*p == '\0') {
        buf[0] = ':';
        os_strncpy(buf + 1, parameters, buf_sz - 1);
        return;
    }

    /* Otherwise, append port number.
     */
    os_strncat(buf, ":", buf_sz);
    osal_int_to_str(nbuf, sizeof(nbuf), default_port_nr);
    os_strncat(buf, nbuf, buf_sz);
}


/**
****************************************************************************************************

  @brief Make ip string from binary address "ip", port number and is_ipv6 flag.
  @anchor osal_make_ip_str

  @param   straddr Pointer to buffer where to store the resulting string,
  @param   straddr_sz IP string buffer size. This should be OSAL_IPADDR_AND_PORT_SZ bytes to allow
           enough space for IPv6 addresses.
  @param   ip Binary IP address, either 4 bytes (IPv4) or 16 bytes (IPv6).
  @param   port_nr TCP port number. If port_nr is 0, port is left out of the resulting string.
  @param   is_ipv6 Flag indicating IPv6. If OS_FALSE, IPv$ is assumed.

****************************************************************************************************
*/
/* NOT USED
void osal_make_ip_str(
    os_char *straddr,
    os_memsz straddr_sz,
    const os_uchar *ip,
    os_int port_nr,
    os_boolean is_ipv6)
{
    os_char nbuf[OSAL_NBUF_SZ + 1];
    if (is_ipv6) {
        *(straddr++) = '[';
        straddr_sz--;
    }

    osal_ip_to_str(straddr, straddr_sz, ip,
        is_ipv6 ? OSAL_IPV6_BIN_ADDR_SZ  : OSAL_IPV4_BIN_ADDR_SZ);
    if (is_ipv6) {
        os_strncat(straddr, "]", straddr_sz);
    }
    if (port_nr) {
        nbuf[0] = ':';
        osal_int_to_str(nbuf + 1, sizeof(nbuf) - 1, port_nr);
        os_strncat(straddr, nbuf, straddr_sz);
    }
}
*/

#endif
