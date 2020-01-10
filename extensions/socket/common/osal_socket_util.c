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
  @anchor osal_socket_get_host_name_and_port

  The osal_socket_get_host_name_and_port() function examines the network address string within 
  long parameter string. If the host name or numeric IP address is specified, the function
  returns pointer to it. If port number is specified in parameter string, the function
  stores it to port.

  @param   parameters Socket parameters, a list string. "addr=host:port" or simply 
           parameter string starting with "host:port", set host name or numeric IP address
           and port number. Host may be in the brackets, like "[host]:port". This is mostly used
           for IP V6 addresses, which themselves may contain colons ':'.
           Marking like ":122" can be used just to specify port number to listen to.
  @param   port_nr Pointer to integer into which to store the port number. If the
           parameters do not specify port number, the port will not be modified.
  @param   buf Pointer to buffer where to store the host name. Buffer of OSAL_HOST_BUF_SZ
           bytes is recommended.
  @param   buf_sz Size of host name buffer.
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
void osal_socket_get_host_name_and_port(
    const os_char *parameters,
    os_int  *port_nr,
    os_char *buf,
    os_memsz buf_sz,
    os_boolean *is_ipv6,
    os_int default_use_flags,
    os_int default_port_nr)
{
    os_memsz n_chars;
    const os_char *value_pos;
    os_char *port_pos;

    *is_ipv6 = OS_FALSE;

	/* Find newtwork address / port within parameter string. 
	 */
    if (parameters == OS_NULL) parameters = "";
	value_pos = osal_str_get_item_value(parameters, "addr", 
		&n_chars, OSAL_STRING_SEARCH_LINE_ONLY);
	if (value_pos == OS_NULL) 
    {
        value_pos = parameters;
        n_chars = os_strlen(value_pos);
    }

    /* Copy parameter value into it. Null terminate the string.
	 */
    if (n_chars  >= buf_sz) n_chars = buf_sz-1;
    os_strncpy(buf, value_pos, n_chars);
	buf[n_chars] = '\0';

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

    /* If we have host address is empty and se are connecting a socket, use local host.
     */
    if (*buf == '\0' && (default_use_flags & OSAL_STREAM_LISTEN) == 0)
    {
        os_strncpy(buf, value_pos, buf_sz);
    }

	/* If starts with bracket, skip it. If host is numeric address which contains colons ':',
       it is IPv6 address. 
	 */
    *is_ipv6 = (os_boolean)(os_strchr(buf, ':') != OS_NULL);
    if (buf[0] == '[') 
    {
        os_memmove(buf, buf+1,  os_strlen(buf+1));
        *is_ipv6 = OS_TRUE;
    }
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
  @param   default_use_flags What socket is used for. This is used to make defaule IP address
           if it is omitted from parameters" string. Set either OSAL_STREAM_CONNECT (0) or
           OSAL_STREAM_LISTEN (0x0100) depending which end of the socket we are preparing.
           Bit fields, can be stream flags directly.
  @param   default_port_nr Port number to embed in parameters string, if port
           number is not already specified.

  @return  None.

****************************************************************************************************
*/
void osal_socket_embed_default_port(
    const os_char *parameters,
    os_char *buf,
    os_memsz buf_sz,
    os_int default_use_flags,
    os_int default_port_nr)
{
    os_char host[OSAL_HOST_BUF_SZ], nbuf[OSAL_NBUF_SZ];
    os_int port_nr;
    os_boolean is_ipv6;

    /* Parse the string.
     */
    osal_socket_get_host_name_and_port(parameters,
        &port_nr, host, sizeof(host), &is_ipv6, default_use_flags, 0 /* use zero to mark not found */);

    /* If we already have the port number in "parameters" string, just use as is.
     */
    if (port_nr != 0)
    {
        os_strncpy(buf, parameters, buf_sz);
        return;
    }

    /* Reformulate "parameters" string with default port number
     */
    *buf = '\0';
    if (is_ipv6) os_strncat(buf, "[", buf_sz);
    os_strncat(buf, host, buf_sz);
    os_strncat(buf, is_ipv6 ? "]:" : ":", buf_sz);
    osal_int_to_str(nbuf, sizeof(nbuf), default_port_nr);
    os_strncat(buf, nbuf, buf_sz);
}


#endif
