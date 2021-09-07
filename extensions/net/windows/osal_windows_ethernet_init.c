/**

  @file    net/windows/osal_windows_ethernet_init.c
  @brief   OSAL stream API implementation for windows sockets.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    24.2.2020

  Ethernet connectivity. Initialization for Windows sockets API.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#ifdef OSAL_WINDOWS
#if OSAL_SOCKET_SUPPORT

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
// #include <Ws2tcpip.h>
// #include <iphlpapi.h>

#include "extensions/net/common/osal_shared_net_info.h"



/**
****************************************************************************************************

  @brief Initialize sockets.
  @anchor osal_socket_initialize

  The osal_socket_initialize() initializes the underlying sockets library.

  @param   nic Pointer to array of network interface structures. Ignored in Windows.
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

    /** Windows socket library version information.
     */
    WSADATA osal_wsadata;

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
        if (nic[i].ip_address[0] == '\0' || !strcmp(nic[i].ip_address, osal_str_asterisk)) continue;

        os_strncpy(sg->nic[sg->n_nics].ip_address, nic[i].ip_address, OSAL_IPADDR_SZ);
        sg->nic[sg->n_nics].receive_udp_multicasts = nic[i].receive_udp_multicasts;
        sg->nic[sg->n_nics].send_udp_multicasts = nic[i].send_udp_multicasts;
        if (++(sg->n_nics) >= OSAL_MAX_NRO_NICS) break;
    }

    /* If socket library is already initialized, do nothing. Double checked here
       for thread synchronization.
     */
    if (osal_global->sockets_shutdown_func == OS_NULL)
    {
        /* Initialize winsock.
         */
        if (WSAStartup(MAKEWORD(2,2), &osal_wsadata))
        {
            osal_debug_error("WSAStartup() failed");
            return;
        }

        /* Mark that socket library has been initialized by setting shutdown function pointer.
           Now the pointer is shared on windows by main program and DLL. If this needs to
           be separated, move sockets_shutdown_func pointer from global structure to
           plain global variable.
         */
        osal_global->sockets_shutdown_func = osal_socket_shutdown;
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
    /* If socket we have shut down function.
     */
    if (osal_global->sockets_shutdown_func)
    {
        /* Initialize winsock.
         */
        if (WSACleanup())
        {
            osal_debug_error("WSACleanup() failed");
            return;
        }

        /* Mark that socket library is no longer initialized.
         */
        osal_global->sockets_shutdown_func = OS_NULL;
    }

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
  has been completed. For Windows there is not much to do, we just return if sockect library
  has been initialized.

  @return  OSAL_SUCCESS if we are connected to a wifi network.
           OSAL_PENDING If currently connecting and have not never failed to connect so far.
           OSAL_STATUS_FAILED No network, at least for now.

****************************************************************************************************
*/
osalStatus osal_are_sockets_initialized(
    void)
{
    return osal_global->sockets_shutdown_func ? OSAL_SUCCESS : OSAL_STATUS_FAILED;
}

#endif
#endif
