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

/** Security configuration, user name, password, trusted parties, certificates
 */
typedef struct osalSecurityConfig
{
/* Security stuff in persistent blocks.
 */
#if OSAL_PERSISTENT_SUPPORT
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
#endif

/* Security stuff in file system.
 */
#if OSAL_FILESYS_SUPPORT
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
    const os_char *client_cert_chain_file;
#endif
}
osalSecurityConfig;

#if OSAL_TLS_SUPPORT

/** Stream interface structure for OpenSLL sockets.
 */
extern const osalStreamInterface osal_tls_iface;

/** Define to get socket interface pointer.
 */
#define OSAL_TLS_IFACE &osal_tls_iface

/* Initialize OSAL sockets library.
 */
void osal_tls_initialize(
    osalNetworkInterface2 *nic,
    os_int n_nics,
    osalSecurityConfig *prm);

/* Shut down OSAL sockets library.
 */
void osal_tls_shutdown(void);

/* Get network and security status.
 */
void osal_tls_get_network_status(
    osalNetworkStatus *net_status,
    os_int nic_nr);

#else

/* No TLS support, define empty macros that we do not need to #ifdef code.
 */
#define osal_tls_initialize(n,c,p)
#define osal_tls_shutdown()

/* No TLS interface, allow build even if the define is used.
 */
#define OSAL_TLS_IFACE OS_NULL

#endif
