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
    osalNetStateNotificationHandler *handler;
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

    /* Call notification handlers.
     */
    for (i = 0; i < OSAL_MAX_ERROR_HANDLERS; i++)
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
