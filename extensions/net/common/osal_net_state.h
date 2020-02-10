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

/** Network state information structure.
 */
typedef struct osalNetworkState
{
    /** General network adapter status:
       - OSAL_SUCCESS = network ready
       - OSAL_PENDING = currently initializing.
       - OSAL_STATUS_NO_WIFI = not connected to a wifi network.
       - OSAL_STATUS_NOT_INITIALIZED = Socket library has not been initialized.
     */
    osalStatus code[OSAL_MAX_NRO_NICS];

    /** Wifi networork connecte flags.
     */
    os_boolean wifi_connected[OSAL_MAX_NRO_WIFI_NETWORKS];

    /** No sertificate chain (transfer automatically?)
     */
    // os_boolean no_cert_chain;

    /** Counts, like number of connected sockets, number of listening sockets, etc.
     */
    os_int count[OSAL_NRO_NET_COUNTS];

    /** Number of socket connect failed.
     */

    /** Number of no access right errors.
     */

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
