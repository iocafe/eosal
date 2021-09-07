/**

  @file    net/linux/osal_linux_ethernet_init.c
  @brief   Network initialization for Linux.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    2.3.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#ifdef OSAL_LINUX
#if OSAL_SOCKET_SUPPORT

#include "extensions/net/common/osal_shared_net_info.h"

/**
****************************************************************************************************

  @brief Initialize sockets.
  @anchor osal_socket_initialize

  The osal_socket_initialize() initializes the underlying sockets library.

  @param   nic Pointer to array of network interface structures. Ignored in Linux.
  @param   n_nics Number of network interfaces in nic array.
  @param   wifi Pointer to array of WiFi network structures. This contains wifi network name (SSID)
           and password (pre shared key) pairs. Can be OS_NULL if there is no WiFi.
  @param   n_wifi Number of wifi networks network interfaces in wifi array.
  @return  None.

****************************************************************************************************
*/
void osal_socket_initialize(
    osalNetworkInterface *nic,
    os_int n_nics,
    osalWifiNetwork *wifi,
    os_int n_wifi)
{
    osalSocketGlobal *sg;
    os_int i;

    /* If socket library is already initialized, do nothing.
     */
    if (osal_global->socket_global) return;

    /* Lock the system mutex to synchronize.
     */
    os_lock();

    /* Allocate global structure
     */
    sg = (osalSocketGlobal*)os_malloc(sizeof(osalSocketGlobal), OS_NULL);
    os_memclear(sg, sizeof(osalSocketGlobal));
    osal_global->socket_global = sg;
    osal_global->sockets_shutdown_func = osal_socket_shutdown;

    /** Copy NIC info for UDP multicasts.
     */
    if (nic) for (i = 0; i < n_nics; i++)
    {
        if (!nic[i].receive_udp_multicasts && !nic[i].send_udp_multicasts) continue;
        if (nic[i].ip_address[0] == '\0' || !os_strcmp(nic[i].ip_address, osal_str_asterisk)) continue;

        os_strncpy(sg->nic[sg->n_nics].ip_address, nic[i].ip_address, OSAL_IPADDR_SZ);
        sg->nic[sg->n_nics].receive_udp_multicasts = nic[i].receive_udp_multicasts;
        sg->nic[sg->n_nics].send_udp_multicasts = nic[i].send_udp_multicasts;
        if (++(sg->n_nics) >= OSAL_MAX_NRO_NICS) break;
    }

    /* End synchronization.
     */
    os_unlock();
}


/**
****************************************************************************************************

  @brief Shut down sockets.
  @anchor osal_socket_shutdown

  The osal_socket_shutdown() shuts down the underlying sockets library.

  @return  None.

****************************************************************************************************
*/
void osal_socket_shutdown(
    void)
{
    /* If we ave global data, release memory.
     */
    if (osal_global->socket_global)
    {
        os_free(osal_global->socket_global, sizeof(osalSocketGlobal));
        osal_global->socket_global = OS_NULL;
    }
}


/**
****************************************************************************************************

  @brief Check if network is initialized.
  @anchor osal_are_sockets_initialized

  The osal_are_sockets_initialized function is called to check if socket library initialization
  has been completed. For Linux there is nothint to do, operating system is in control of this.

  @return  OSAL_SUCCESS if we are connected to a wifi network (always returned in Linux).
           OSAL_PENDING If currently connecting and have not never failed to connect so far.
           OSAL_STATUS_FAILED No network, at least for now.

****************************************************************************************************
*/
osalStatus osal_are_sockets_initialized(
    void)
{
    return osal_global->socket_global ? OSAL_SUCCESS : OSAL_STATUS_FAILED;
}

#endif
#endif
