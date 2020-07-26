/**

  @file    net/common/osal_dns.h
  @brief   Resolve host name or IP address string.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    3.3.2020

  Socket speficic function prototypes and definitions to implement OSAL stream API for sockets.
  OSAL stream API is abstraction which makes streams (including sockets) look similar to upper
  levels of code, regardless of operating system or network library implementation.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_DNS_H_
#define OSAL_DNS_H_
#include "eosalx.h"

#if OSAL_SOCKET_SUPPORT

/* Get host's binary address by name or IP address string.
 */
osalStatus osal_gethostbyname(
    const os_char *name,
    os_char *addr,
    os_memsz addr_sz,
    os_boolean *is_ipv6,
    os_int default_use_flags);

#endif
#endif
