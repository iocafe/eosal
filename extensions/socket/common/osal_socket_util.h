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

/* Get host and port from network address string (osal_socket_util.c).
 */
osalStatus osal_socket_get_ip_and_port(
    const os_char *parameters,
    os_char *addr,
    os_memsz addr_sz,
    os_int  *port_nr,
    os_boolean *is_ipv6,
    os_int default_use_flags,
    os_int default_port_nr);

/* If port number is not specified in "parameters" string, then embed defaut port number.
 */
void osal_socket_embed_default_port(
    const os_char *parameters,
    os_char *buf,
    os_memsz buf_sz,
    os_int default_port_nr);
