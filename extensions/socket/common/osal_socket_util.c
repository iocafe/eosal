/**

  @file    socket/common/osal_socket_util.c
  @brief   Socket helper functions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

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

  The osal_socket_get_ip_and_port() function examines the network address string within
  long parameter string. If the host name or numeric IP address is specified, the function
  returns pointer to it. If port number is specified in parameter string, the function
  stores it to port.

  @param   parameters Socket parameters, a parameter string starting with "host:port", set
           host name or numeric IP address and port number. Host may be in the brackets,
           like "[host]:port". This is mostly used for IP V6 addresses, which themselves
           may contain colons ':'.
           Marking like ":122" can be used just to specify port number to listen to.
  @param   addr Pointer where to store the binary IP address. IP address is stored in
           network byte order (most significant byte first). Either 4 or 16 bytes are stored
           depending if this is IPv4 or IPv6 address. Entire buffer is anythow cleared.
  @param   addr_sz Address buffer size in bytes. This should be minimum 16 bytes to allow
           storing IPv6 address.
  @param   is_ipv6 Pointer to boolean to set to OS_TRUE if this is IPv6 address or OE_FALSE
           if this is IPv4 address.
  @param   port_nr Pointer to integer into which to store the port number. If the
           parameters do not specify port number, the port will not be modified.
  @param   is_ipv6 Flag to set if IP v6 address has been detected. Notice that this works only
           for numeric IP v6addresses: if host name refers to IPv6 (no numeric address),
           IPv6 is not detected here but needs to be checked after resolving name with DNS.
  @param   default_use_flags What socket is used for. This is used to make defaule IP address
           if it is omitted from parameters" string. Set either OSAL_STREAM_CONNECT (0) or
           OSAL_STREAM_LISTEN (0x0100) depending which end of the socket we are preparing.
           Bit fields, can be stream flags directly.
  @param   default_port_nr Port number to embed in parameters string, if port
           number is not already specified.

  @return  None.

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
    os_char *port_pos;

    os_strncpy(buf, parameters, sizeof(buf));

	/* Search for port number position. If this is ipv4 address or unknown address type, 
       the port number is separared by ':'. In future, if this is ipv6 address, port number
       may be separated by '#'. The '#' may change, after we get more familiar with ipv6
       addresses writing convention.
	 */
	port_pos = os_strchr(buf, ']');
    if (port_pos)
    {
        *(port_pos++) = '\0';
        if (*(port_pos++) != ':') port_pos = OS_NULL;
    }
    else
    {
		port_pos = os_strchr(buf, ':');
        if (port_pos) *(port_pos++) = '\0';
    }

	/* Parse port number from string.
	 */
	if (port_pos)
	{
        *port_nr = (os_int)osal_str_to_int(port_pos, OS_NULL);
	}
    else
    {
        *port_nr = default_port_nr;
    }

    /* Convert to binary IP. If starts with bracket, skip it.
	 */
    return osal_gethostbyname(buf[0] == '[' ? buf + 1 : buf, addr, addr_sz,
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

    /* Otherwise, append port number.
     */
    os_strncat(buf, ":", buf_sz);
    osal_int_to_str(nbuf, sizeof(nbuf), default_port_nr);
    os_strncat(buf, nbuf, buf_sz);
}

#endif
