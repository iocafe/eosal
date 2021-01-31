/**

  @file    tls/common/osal_tls.h
  @brief   OSAL streams, TLS stream API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.1.2020

  The osalSecurityConfig structure speficies where the server certificate, private server key,
  and client certificate chain are located.

  These are used when a secure connection is establised: Server certificate and key are stored
  in server, client certificate chain in client. The client received server certificate when
  a connection is being established. It decides if the server can be trusted by matching server
  certificate to certificate chain based it already has. Server key is the secret (which never
  leaves server) used to make sure that the server certificate really belongs to the server.
  This complex hand shake is taken care by TLS library.

  Server certificate and client certificate chain are public information and can be published
  to anyone. The seacret to be protected here is the server's private key.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_TLS_H_
#define OSAL_TLS_H_
#include "eosalx.h"

/** Security configuration, user name, password, trusted parties, certificates
 */
typedef struct osalSecurityConfig
{
/* Security stuff in persistent blocks.
 */
    /** Server certificate in persistent block. Value OS_PBNR_SERVER_CERT (6) indicates
        that server certificate is stored in persisent block 6. Value OS_PBNR_UNKNOWN (0)
        if server certificate is not stored in persistent block.
     */
    short server_cert_pbnr;

    /** Server key in persistent block. Value OS_PBNR_SERVER_KEY (4) if the server key
        is stored in persisent block 4. Value OS_PBNR_UNKNOWN (0) if server key is not stored
        in persistent block.
     */
    short server_key_pbnr;

    /** Client certificate chain in persistent block. Value OS_PBNR_CLIENT_CERT_CHAIN (7) if
        in persistent block 7, value OS_PBNR_UNKNOWN (0) if not stored in persistent block.
     */
    short client_cert_chain_pbnr;

/* Security stuff in file system.
 */
    /** Path to directory containing certificates and keys. OS_NULL to use testing default.
     */
    const os_char *certs_dir;

    /** Server certificate file (PEM)
     */
    const os_char *server_cert_file;

    /** Server key
     */
    const os_char *server_key_file;

    /** Root certificate.
     */
    const os_char *root_cert_file;

    /** Client certificate chain file (PEM, bundle)
     */
    const os_char *trusted_cert_file;
}
osalSecurityConfig;


/* Default TLS socket port number for IOCOM.
 */
#define IOC_DEFAULT_TLS_PORT 6369
#define IOC_DEFAULT_TLS_PORT_STR "6369"


#if OSAL_TLS_SUPPORT

/** Stream interface structure for OpenSLL sockets.
 */
extern OS_CONST_H osalStreamInterface osal_tls_iface;

/** Define to get socket interface pointer.
 */
#define OSAL_TLS_IFACE &osal_tls_iface

/* Initialize OSAL sockets library.
 */
void osal_tls_initialize(
    osalNetworkInterface *nic,
    os_int n_nics,
    osalWifiNetwork *wifi,
    os_int n_wifi,
    osalSecurityConfig *prm);

/* Shut down OSAL sockets library.
 */
void osal_tls_shutdown(void);

#else

/* No TLS support, define empty macros that we do not need to #ifdef code.
 */
#define osal_tls_initialize(n,c,w,d,p)
#define osal_tls_shutdown()

/* No TLS interface, allow build even if the define is used.
 */
#define OSAL_TLS_IFACE OS_NULL

#endif

#endif
