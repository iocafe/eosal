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

/** Network state information structure.
 */
typedef struct osalNetworkState
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
osalNetworkState;


// Set state function

// Change callback type

// Set callback function
