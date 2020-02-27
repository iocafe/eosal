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

  The osal_socket_get_ip_and_port() function converts the network address string used by 
  eosal library to binary IP address and port number.

  @param   parameters Socket parameters, a parameter string like  "host:port". IPv6 host 
           name should be within square brackets, like "[host]:port". "Host" string can
           be either host name or IP address. Asterix '*' as host name, or empty host name
           mean default address (default_use_flags). Asterix '*' or empty port number
           means default port. Marking like ":122" can be used just to specify port number 
           to listen to.
  @param   addr Pointer where to store the binary IP address. IP address is stored in
           network byte order (most significant byte first). Either 4 or 16 bytes are stored
           depending if this is IPv4 or IPv6 address. Entire buffer is anythow cleared.
  @param   addr_sz Address buffer size in bytes. This should be minimum 16 bytes to allow
           storing IPv6 address.
  @param   is_ipv6 Pointer to boolean to set to OS_TRUE if this is IPv6 address or OE_FALSE
           if this is IPv4 address.
  @param   port_nr Pointer to integer into which to store the port number. 
  @param   is_ipv6 Flag to set if IP v6 address has been selected. 
  @param   default_use_flags What socket is used for. This is used to make defaule IP address
           if it is omitted from parameters" string. Set either OSAL_STREAM_CONNECT (0) or
           OSAL_STREAM_LISTEN depending which end of the socket we are preparing.
           OSAL_STREAM_MULTICAST if we are using the address for multicasts.
           Bit fields, can be stream flags as they are, extra flags are ignored.
  @param   default_port_nr Default port number to return if port number is not specified
           in parameters string.

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

	/* Terminate address with '\0' character, search for port number position within the string.  
	 */
	port_pos = os_strchr(buf, ']');
    if (port_pos)
    {
        *(port_pos++) = '\0';
        if (*(port_pos++) != ':') port_pos = "";
    }
    else
    {
		port_pos = os_strchr(buf, ':');
        if (port_pos) *(port_pos++) = '\0';
        else port_pos = "";
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
    *is_ipv6 = buf[0] == '[';

    /* Asterix '*' as host name is same as empty host name. We will still proceed to call
       osal_gethostbyname to get specific operating system specific addr settings.
     */
    if (os_strchr(buf, '*')) {
        buf[0] = '\0';
    }

    /* Convert to binary IP. If starts with bracket, skip it. This is operating system specific.
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
