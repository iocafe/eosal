/**

  @file    socket/common/osal_net_state.h
  @brief   OSAL stream API for sockets.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.2.2020

  Current network state.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

struct osalNetworkState;

/** Network state change notification handler function type.
 */
typedef void osal_net_state_notification_handler(
    struct osalNetworkState *net_state,
    void *context);

/** Structure for storing notification handler function pointer and context.
 */
typedef struct osalNetStateNotificationHandler
{
    osal_net_state_notification_handler *func;
    void *context;
}
osalNetStateNotificationHandler;

/** Maximum number of notification handlers.
 */
#ifndef OSAL_MAX_NET_STATE_NOTIFICATION_HANDLERS
#define OSAL_MAX_NET_STATE_NOTIFICATION_HANDLERS 2
#endif

/** Enumeration of counts, like number of connected sockets, number of listening sockets, etc.
 */
typedef enum
{
    OSAL_NRO_CONNECTED_SOCKETS,
    OSAL_NRO_LISTENING_SOCKETS,
    OSAL_NRO_UDP_SOCKETS,

    OSAL_NRO_NET_COUNTS
}
osalNetCountIx;

/** Enumeration of network state items (other than counts).
 */
typedef enum
{
    OSAL_NS_NIC_STATE,
    OSAL_NS_NIC_IP_ADDR,
    OSAL_NS_NETWORK_USED,
    OSAL_NS_NETWORK_CONNECTED,
    OSAL_NS_WIFI_NETWORK_NAME,
    OSAL_NS_WIFI_PASSWORD,
    OSAL_NS_LIGHTHOUSE_STATE,
    OSAL_NS_NO_CERT_CHAIN
}
osalNetStateItem;


/** Enumeration of network state items (other than counts).
 */
typedef enum
{
    OSAL_LIGHTHOUSE_NOT_USED = 0,
    OSAL_LIGHTHOUSE_OK = 1,
    OSAL_LIGHTHOUSE_NOT_VISIBLE = 2,
    OSAL_NO_LIGHTHOUSE_FOR_THIS_IO_NETWORK = 3
}
osaLightHouseClientState;

/** Network state information structure.
 */
typedef struct osalNetworkState
{
#if OSAL_SOCKET_SUPPORT
    /** Network adapter status:
       - OSAL_SUCCESS = network ready
       - OSAL_PENDING = currently initializing.
       - OSAL_STATUS_NO_WIFI = not connected to a wifi network.
       - OSAL_STATUS_NOT_INITIALIZED = Socket library has not been initialized.
     */
    osalStatus nic_code[OSAL_MAX_NRO_NICS];

    /** Network adapter IP address:
     */
    os_char nic_ip[OSAL_MAX_NRO_NICS][OSAL_IPADDR_SZ];

    /** Wifi network name (SSID).
     */
    os_char wifi_network_name[OSAL_MAX_NRO_WIFI_NETWORKS][OSAL_WIFI_PRM_SZ];

    /** Wifi network password (PSK).
     */
    os_char wifi_network_password[OSAL_MAX_NRO_WIFI_NETWORKS][OSAL_WIFI_PRM_SZ];

    /** Ethwenet or Wifi network used flag.
     */
    os_boolean network_used;

    /** Ethwenet or Wifi network connected flag.
     */
    os_boolean network_connected;

    /** No sertificate chain (transfer automatically?)
     */
    os_boolean no_cert_chain;

    /** Light house client state.
     */
    osaLightHouseClientState lighthouse_state;
#endif

    /** Counts, like number of connected sockets, number of listening sockets, etc.
     */
    os_int count[OSAL_NRO_NET_COUNTS];

    /** Notification handler functions.
     */
    osalNetStateNotificationHandler notification_handler[OSAL_MAX_NET_STATE_NOTIFICATION_HANDLERS];
}
osalNetworkState;


/* Initialize network state (does nothing if already initialized)
 */
void osal_initialize_net_state(void);

/* Add net state change notification handler (function to be called when net state changes).
 */
osalStatus osal_add_network_state_notification_handler(
    osal_net_state_notification_handler *func,
    void *context,
    os_short reserved);

/* Set network state item. For example called by TLS socket wrapper to inform that we do not
   have client certificate chain.
 */
void osal_set_network_state_int(
    osalNetStateItem item,
    os_int index,
    os_int value);

/* Get network state item.
 */
os_int osal_get_network_state_int(
    osalNetStateItem item,
    os_int index);

/* Set network state item which can contain string, like OSAL_NS_NIC_IP_ADDR.
 */
void osal_set_network_state_str(
    osalNetStateItem item,
    os_int index,
    const os_char *str);

/* Get network state string item.
 */
void osal_get_network_state_str(
    osalNetStateItem item,
    os_int index,
    os_char *str,
    os_memsz str_sz);

