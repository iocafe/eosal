/**

  @file    tls/common/osal_tls.h
  @brief   OSAL sockets API OpenSSL implementation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    20.8.2019

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_OPENSSL_INCLUDED
#define OSAL_OPENSSL_INCLUDED


typedef struct osalTLSParam
{
    const os_char *certfile;
    const os_char *keyfile;
}
osalTLSParam;

#if OSAL_TLS_SUPPORT

/** Stream interface structure for OpenSLL sockets.
 */
extern const osalStreamInterface osal_tls_iface;

/** Define to get socket interface pointer.
 */
#define OSAL_TLS_IFACE &osal_tls_iface

/* TLS initialized flag.
 */
extern os_boolean osal_tls_initialized;

/* Initialize OSAL sockets library.
 */
void osal_tls_initialize(
    osalNetworkInterface *nic,
    os_int n_nics,
    osalTLSParam *prm);

/* Shut down OSAL sockets library.
 */
void osal_tls_shutdown(void);

#else

/* No socket support, define empty macros that we do not need to #ifdef code.
 */
#define osal_tls_initialize(n,c,p)
#define osal_tls_shutdown()

/* No TLS interface, allow build even if the define is used.
 */
#define OSAL_TLS_IFACE OS_NULL

#endif
#endif
