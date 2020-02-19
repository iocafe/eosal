/**

  @file    socket/common/osal_net_config.h
  @brief   OSAL stream API for sockets.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.2.2020

  Network configuration structures and defines.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

/* Default buffer size for host (computer) name and port, etc.
 */
#define OSAL_HOST_BUF_SZ 64

/* Default buffer size for IP address or host name only.
 */
#define OSAL_IPADDR_SZ 46

/* Default buffer size for MAC address string.
 */
#define OSAL_MAC_SZ 18

/* Size for WiFi network name and password.
 */
#define OSAL_WIFI_PRM_SZ 16

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
typedef struct osalWifiNetwork
{
    const os_char *wifi_net_name;
    const os_char *wifi_net_password;
}
osalWifiNetwork;

/** Network interface configuration structure.
 */
typedef struct osalNetworkInterface
{
    const os_char *host_name;
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
}
osalNetworkInterface;

/** Flat structure to for save into persistWifi network name and password.
 */
typedef struct osalWifiNetworkBuf
{
    os_char wifi_net_name[OSAL_WIFI_PRM_SZ];
    os_char wifi_net_password[OSAL_WIFI_PRM_SZ];
}
osalWifiNetworkBuf;

/** Structure to save Wifi configuration as persistent block
    numer OS_PBNR_WIFI.
 */
typedef struct osalWifiPersistent
{
    osalWifiNetworkBuf wifi[OSAL_MAX_NRO_WIFI_NETWORKS];
}
osalWifiPersistent;
