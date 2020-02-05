/**

  @file    socket/common/osal_socket.h
  @brief   OSAL stream API for sockets.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Socket speficic function prototypes and definitions to implement OSAL stream API for sockets.
  OSAL stream API is abstraction which makes streams (including sockets) look similar to upper
  levels of code, regardless of operating system or network library implementation.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

/** Stream interface structure for sockets.
 */
#if OSAL_SOCKET_SUPPORT
#if OSAL_FUNCTION_POINTER_SUPPORT
extern const osalStreamInterface osal_socket_iface;
#endif
#endif

/* Default socket port number for IOCOM.
 */
#define IOC_DEFAULT_SOCKET_PORT 6368
#define IOC_DEFAULT_SOCKET_PORT_STR "6368"
#define IOC_DEFAULT_TLS_PORT 6369
#define IOC_DEFAULT_TLS_PORT_STR "6369"

/* Maximum number of socket streams to pass as an argument to osal_socket_select().
 */
#define OSAL_SOCKET_SELECT_MAX 8

/* Default buffer size for host (computer) name and port, etc.
 */
#define OSAL_HOST_BUF_SZ 64

/* Default buffer size for IP address or host name only.
 */
#define OSAL_IPADDR_SZ 40

/* Default buffer size for MAC address string.
 */
#define OSAL_MAC_SZ 18

/* Size for WiFi network name and password.
 */
#define OSAL_WIFI_PRM_SZ 16


#if OSAL_SOCKET_SUPPORT

/** Define to get socket interface pointer. The define is used so that this can
    be converted to function call.
 */
#define OSAL_SOCKET_IFACE &osal_socket_iface


/* Number of network interfaces that can be configured by eosal. This doesn't limit
   number of network interfaces when operating system like Linux or Windows manages these.
 */
#ifndef OSAL_MAX_NRO_NICS
#define OSAL_MAX_NRO_NICS 2
#endif

/* Maximum number Number of network interfaces that should be supported troughout the code.
 */
#ifndef OSAL_MAX_NRO_WIFI_NETWORKS
#define OSAL_MAX_NRO_WIFI_NETWORKS 2
#endif

/** Wifi network name and password.
 */
typedef struct
{
    const os_char *wifi_net_name;
    const os_char *wifi_net_password;
}
osalWifiNetwork;

/** Network interface configuration structure.
 */
typedef struct osalNetworkInterface
{
    /** Network interface name
     */
    const os_char *nic_name;

    const os_char *host_name;

    /* We keep network setup in global variables for micro controllers.
     */
    const os_char *ip_address;
    const os_char *subnet_mask;
    const os_char *gateway_address;
    const os_char *dns_address;
    const os_char *dns_address_2;

    /* Locally administered MAC address ranges safe for testing: x2:xx:xx:xx:xx:xx,
       x6:xx:xx:xx:xx:xx, xA:xx:xx:xx:xx:xx and xE:xx:xx:xx:xx:xx
    */
    const os_char *mac;
    os_boolean no_dhcp;

    /** WiFi network configuration (SSID/pre shared key pairs).
     */
    /* osalWifiNetwork wifinet[OSAL_MAX_NRO_WIFI_NETWORKS]; */
}
osalNetworkInterface;


/** Network status information structure.
 */
typedef struct osalNetworkStatus
{
    /** General network status:
       - OSAL_SUCCESS = network ready
       - OSAL_STATUS_PENDING = currently initializing.
       - OSAL_STATUS_NO_WIFI = not connected to a wifi network.
       - OSAL_STATUS_NOT_INITIALIZED = Socket library has not been initialized.
     */
    osalStatus code;

    /** Wifi networork connecte flags.
     */
    os_boolean wifi_connected[OSAL_MAX_NRO_WIFI_NETWORKS];

    /** No sertificate chain (transfer automatically?)
     */
    os_boolean no_cert_chain;
}
osalNetworkStatus;


/* Socket library initialized flag.
 */
extern os_boolean osal_sockets_initialized;


/** 
****************************************************************************************************

  @name OSAL Socket Functions.

  These functions implement sockets as OSAL stream. These functions can either be called 
  directly or through stream interface.

****************************************************************************************************
 */
/*@{*/

/* Open socket.
 */
osalStream osal_socket_open(
    const os_char *parameters,
    void *option,
    osalStatus *status,
    os_int flags);

/* Close socket.
 */
void osal_socket_close(
    osalStream stream,
    os_int flags);

/* Accept connection from listening socket.
 */
osalStream osal_socket_accept(
    osalStream stream,
    os_char *remote_ip_addr,
    os_memsz remote_ip_addr_sz,
    osalStatus *status,
    os_int flags);

/* Flush written data to socket.
 */
osalStatus osal_socket_flush(
    osalStream stream,
    os_int flags);

/* Write data to socket.
 */
osalStatus osal_socket_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags);

/* Read data from socket.
 */
osalStatus osal_socket_read(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags);

/* Get socket parameter.
 */
os_long osal_socket_get_parameter(
    osalStream stream,
    osalStreamParameterIx parameter_ix);

/* Set socket parameter.
 */
void osal_socket_set_parameter(
    osalStream stream,
    osalStreamParameterIx parameter_ix,
    os_long value);

/* Wait for new data to read, time to write or operating system event, etc.
 */
#if OSAL_SOCKET_SELECT_SUPPORT
osalStatus osal_socket_select(
    osalStream *streams,
    os_int nstreams,
    osalEvent evnt,
    osalSelectData *selectdata,
    os_int timeout_ms,
    os_int flags);
#endif

/* Initialize OSAL sockets library.
 */
void osal_socket_initialize(
    osalNetworkInterface *nic,
    os_int n_nics,
    osalWifiNetwork *wifi,
    os_int n_wifi);

/* Shut down OSAL sockets library.
 */
void osal_socket_shutdown(void);

/* Are sockets initialized (most important with wifi, call always when opening the
   socket to maintain wifi state).
 */
osalStatus osal_are_sockets_initialized(
    void);

/* Get network status, like is wifi connected?
 */
void osal_socket_get_network_status(
    osalNetworkStatus *net_status,
    os_int nic_nr);

/* Keep the sockets library alive.
 */
#if OSAL_SOCKET_MAINTAIN_NEEDED
void osal_socket_maintain(void);
#else
#define osal_socket_maintain()
#endif


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

/*@}*/

#else

/* No socket support, define empty macros that we do not need to #ifdef code.
 */
#define osal_socket_initialize(n,c,w,d)
#define osal_socket_shutdown()
#define osal_socket_maintain()

/* No socket interface, allow build even if the define is used.
 */
#define OSAL_SOCKET_IFACE OS_NULL

#endif
