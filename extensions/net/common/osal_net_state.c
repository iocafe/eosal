/**

  @file    socket/common/osal_net_state.c
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
#include "eosalx.h"

/* Statically allocated memory for network state. Accessed trough global
   pointer osal_global->net_state.
 */
static osalNetworkState static_net_state;

typedef struct
{
    os_short inc_status;
    os_short dec_status;
    osalNetCountIx count_ix;
}
osalStatToNetCountIx;

static const osalStatToNetCountIx stat_to_count_ix[] = {
    {OSAL_SOCKET_CONNECTED, OSAL_SOCKET_DISCONNECTED, OSAL_NRO_CONNECTED_SOCKETS},
    {OSAL_LISTENING_SOCKET_CONNECTED, OSAL_LISTENING_SOCKET_DISCONNECTED, OSAL_NRO_LISTENING_SOCKETS},
    {OSAL_UDP_SOCKET_CONNECTED, OSAL_UDP_SOCKET_DISCONNECTED, OSAL_NRO_UDP_SOCKETS}};

#define N_STAT_TO_COUNT (sizeof(stat_to_count_ix)/sizeof(osalStatToNetCountIx))

/* Forward referred static functions.
 */
static void osal_net_state_handler(
    osalErrorLevel level,
    const os_char *module,
    os_int code,
    const os_char *description,
    void *context);

static void osal_call_network_state_notification_handlers(
    void);

/* Initialize network state (does nothing if already initialized)
 */
void osal_initialize_net_state(void)
{
    /* If already initialized, do nothing.
     */
    if (osal_global->net_state) return;

    /* Set global network state pointer.
     */
    osal_global->net_state = &static_net_state;

    /* Set error handler callback function to get informaion from error handling.
     */
    osal_set_error_handler(osal_net_state_handler, &static_net_state,
        OSAL_ADD_ERROR_HANDLER|OSAL_SYSTEM_ERROR_HANDLER);
}


/* Error handler to move information provided by error handler callbacks to network state structure.
 */
static void osal_net_state_handler(
    osalErrorLevel level,
    const os_char *module,
    os_int code,
    const os_char *description,
    void *context)
{
    osalNetworkState *ns;
    const osalStatToNetCountIx *sti;
    int i;
    os_boolean changed;

    /* If not oesal/iocom generated, do nothing.
     */
    if (os_strcmp(module, eosal_mod) && os_strcmp(module, "iocom")) return;

    ns = (osalNetworkState*)context;
    changed = OS_FALSE;

    for (i = 0; i < N_STAT_TO_COUNT; i++)
    {
        sti = &stat_to_count_ix[i];

        if (code == sti->inc_status) {
            if (level == OSAL_CLEAR_ERROR) {
                if (ns->count[sti->count_ix])
                {
                    ns->count[sti->count_ix] = 0;
                    changed = OS_TRUE;
                }
            }
            else {
                ns->count[sti->count_ix]++;
                changed = OS_TRUE;
            }
        }
        else if (code == sti->dec_status)
        {
            if (level == OSAL_CLEAR_ERROR) {
                if (ns->count[sti->count_ix]) {
                    ns->count[sti->count_ix] = 0;
                }
            }
            else {
                ns->count[sti->count_ix]--;
                changed = OS_TRUE;
            }
        }
    }
    if (!changed) return;

    osal_call_network_state_notification_handlers();
}

static void osal_call_network_state_notification_handlers(
    void)
{
    osalNetStateNotificationHandler *handler;
    osalNetworkState *ns;
    int i;

    ns = osal_global->net_state;
    if (ns == OS_NULL) return;

    /* Call notification handlers.
     */
    for (i = 0; i < OSAL_MAX_NET_STATE_NOTIFICATION_HANDLERS; i++)
    {
        handler = &ns->notification_handler[i];
        if (handler->func != OS_NULL)
        {
            handler->func(ns, handler->context);
        }
    }
}


/**
****************************************************************************************************

  @brief Add net state change notification handler (function to be called when net state changes).
  @anchor osal_add_network_state_notification_handler

  The osal_add_network_state_notification_handler() function saves pointer to custom
  notification handler function and application context pointer. After this, the notification
  handler function is called when net state changes.

  SETTING ERROR HANDLERS IS NOT MULTITHREAD SAFE AND THUS ERROR HANDLER FUNCTIONS NEED BE
  SET BEFORE THREADS ARE COMMUNICATION, ETC THREADS WHICH CAN REPORT ERRORS ARE CREATED.

  @param   func Pointer to application provided handler function.
  @param   context Application specific pointer, to be passed to notification handler when it
           is called.
  @param   reserved Reserver for future, set 0.

  @return  If successfull, the function returns OSAL_SUCCESS. If there are already maximum
           number of notification handlers (OSAL_MAX_NET_STATE_NOTIFICATION_HANDLERS), the
           function returns OSAL_STATUS_FAILED.

****************************************************************************************************
*/
osalStatus osal_add_network_state_notification_handler(
    osal_net_state_notification_handler *func,
    void *context,
    os_short reserved)
{
    osalNetStateNotificationHandler *handler;
    int i;

    /* Make sure that network state is initialized.
     */
    osal_initialize_net_state();

    /* Add notification handler.
     */
    for (i = 0; i < OSAL_MAX_NET_STATE_NOTIFICATION_HANDLERS; i++)
    {
        handler = &osal_global->net_state->notification_handler[i];
        if (handler->func == OS_NULL)
        {
            handler->func = func;
            handler->context = context;
            return OSAL_SUCCESS;
        }
    }

    /* Too many error handlers.
     */
    return OSAL_STATUS_FAILED;
}

/* Set network state item. For example called by TLS socket wrapper to inform that we do not
   have client certificate chain.
   @param   item OSAL_NS_NIC_STATE, OSAL_NS_WIFI_CONNECTED, OSAL_NS_NO_CERT_CHAIN
 */
void osal_set_network_state_int(
    osalNetStateItem item,
    os_int index,
    os_int value)
{
    osalNetworkState *ns;

    ns = osal_global->net_state;
    if (ns == OS_NULL) return;

    switch (item)
    {
#if OSAL_SOCKET_SUPPORT
        case OSAL_NS_NIC_STATE:
            if (index < 0 || index >= OSAL_MAX_NRO_NICS) return;
            if (ns->nic_code[index] == (os_boolean)value) return;
            ns->nic_code[index] = (os_boolean)value;
            break;

        case OSAL_NS_WIFI_CONNECTED:
            if (index < 0 || index >= OSAL_MAX_NRO_WIFI_NETWORKS) return;
            if (ns->wifi_connected[index] == (os_boolean)value) return;
            ns->wifi_connected[index] = (os_boolean)value;
            break;

        case OSAL_NS_NO_CERT_CHAIN:
            if (ns->no_cert_chain == (os_boolean)value) return;
            ns->no_cert_chain = (os_boolean)value;
            break;
#endif
        default:
            return;
    }

    osal_call_network_state_notification_handlers();
}


/* Get network state item.
 */
os_int osal_get_network_state_int(
    osalNetStateItem item,
    os_int index)
{
    osalNetworkState *ns;
    os_int rval;

    ns = osal_global->net_state;
    if (ns == OS_NULL) return 0;

    rval = 0;
    switch (item)
    {
#if OSAL_SOCKET_SUPPORT
        case OSAL_NS_NIC_STATE:
            if (index < 0 || index >= OSAL_MAX_NRO_NICS) break;
            rval = ns->nic_code[index];
            break;

        case OSAL_NS_WIFI_CONNECTED:
            if (index < 0 || index >= OSAL_MAX_NRO_WIFI_NETWORKS) break;
            rval = ns->wifi_connected[index];
            break;

        case OSAL_NS_NO_CERT_CHAIN:
            rval = ns->no_cert_chain;
            break;
#endif
        default:
            break;
    }

    return rval;
}

/* Set network state item which can contain string, like OSAL_NS_NIC_IP_ADDR.
 */
void osal_set_network_state_str(
    osalNetStateItem item,
    os_int index,
    const os_char *str)
{
    osalNetworkState *ns;

    ns = osal_global->net_state;
    if (ns == OS_NULL) return;

    switch (item)
    {
#if OSAL_SOCKET_SUPPORT
        case OSAL_NS_NIC_IP_ADDR:
            if (index < 0 || index >= OSAL_MAX_NRO_NICS) return;
            if (!os_strcmp(str, ns->nic_ip[index])) return;
            os_strncpy(ns->nic_ip[index], str, OSAL_IPADDR_SZ);
            break;
#endif
        default:
            return;
    }

    osal_call_network_state_notification_handlers();
}

/* Get network state string item.
 */
void osal_get_network_state_str(
    osalNetStateItem item,
    os_int index,
    os_char *str,
    os_memsz str_sz)
{
    osalNetworkState *ns;
    os_int rval;

    if (str && str_sz) *str = '\0';
    ns = osal_global->net_state;
    if (ns == OS_NULL || str == OS_NULL) return;

    rval = 0;
    switch (item)
    {
#if OSAL_SOCKET_SUPPORT
        case OSAL_NS_NIC_IP_ADDR:
            if (index < 0 || index >= OSAL_MAX_NRO_NICS) break;
            os_strncpy(str, ns->nic_ip[index], str_sz);
            break;
#endif
        default:
            break;
    }
}
