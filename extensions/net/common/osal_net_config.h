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

/** Default buffer size for host (computer) name and port, etc.
 */
#define OSAL_HOST_BUF_SZ 64

/** Default buffer size for IP address. 46 is typical default, we use 48 since we use angle
    brackets to mark IPv6 addressess.
 */
#define OSAL_IPADDR_SZ 48

/** Default buffer size for IP address and port number. This is IP address size, separating ':'
    and 5 digits for port number.
 */
#define OSAL_IPADDR_AND_PORT_SZ (OSAL_IPADDR_SZ + 6)

/** Default buffer size for MAC address string.
 */
#define OSAL_MAC_SZ 18

/** Size for WiFi network name and password.
 */
#define OSAL_WIFI_PRM_SZ 16

/** Device number as string, buffer size.
 */
#define OSAL_DEVICE_NR_STR_SZ 8

/** Maximum binary IP address size. 4 bytes for IPv4 and 16 bytes for IPv6.
 */
#define OSAL_IP_BIN_ADDR_SZ 16
#define OSAL_IPV4_BIN_ADDR_SZ 4
#define OSAL_IPV6_BIN_ADDR_SZ 16

/** Number of network interfaces that can be configured by eosal. This doesn't limit
    number of network interfaces when operating system like Linux or Windows manages these.
 */
#ifndef OSAL_MAX_NRO_NICS
#if OSAL_MICROCONTROLLER
#define OSAL_MAX_NRO_NICS 2
#else
#define OSAL_MAX_NRO_NICS 6
#endif
#endif

/** Maximum number Number of network interfaces that should be supported troughout the code.
 */
#ifndef OSAL_MAX_NRO_WIFI_NETWORKS
#define OSAL_MAX_NRO_WIFI_NETWORKS 2
#endif

/** Maximum network name string length.
 */
#define OSAL_NETWORK_NAME_SZ 24

/** Wifi network name and password.
 */
typedef struct osalWifiNetwork
{
    /** Wifi network name (SSID) to connect to, for example "bean24".
     */
    const os_char *wifi_net_name;

    /** Wifi network password is same as pre shared key (PSK).
     */
    const os_char *wifi_net_password;
}
osalWifiNetwork;


/** Network interface configuration structure. Most of network configuration
 *  parameters are used only for microcontrollers, while in Windows, Linux, etc.
    the operating system takes care about network interface configuration.

    Flags to enable/disable sending and receiving UDP multicasts is meaningfull
    in Linux/Windows environment, but not in microcontrollers with single network
    adapter). If thiese are set, "ip_address" parameter is used to pass
    network interface for multicasts.
 */
typedef struct osalNetworkInterface
{
    /*  const os_char *host_name;  to be implemented */

    /** Network address, like "192.168.1.220". Ignored if no_dhcp is OS_FALSE (0).
        Set by "ip":"192.168.1.217" in JSON configuration.
     */
    const os_char *ip_address;

    /** Subnet mask selects which addressess are in same network segment and which
        need to be sent tough gateway, for example "255.255.255.0".
        Set by "subnet":"255.255.255.0" in JSON configuration.
     */
    const os_char *subnet_mask;

    /** Gateway address. Address to use to get from local network segment
        to world outside. At home network, this can be your DSL modem's address
        in your local network. Set by "gateway":"192.168.1.254" in JSON configuration.
     */
    const os_char *gateway_address;

    /** Domain name server address. If host names, like "MYCOMPUTER" are used
        instead of numeric IP address, this is address of server computer
        which resolves that "MYCOCOMPUTER" actually means "192.168.1.220".
        For example "8.8.8.8" is Google's open DNS server. Set by "dns":"8.8.4.4"
        in JSON configuration.
     */
    const os_char *dns_address;

    /** Second domain name server, to be used in case first one is down.
        Set by "dns":"8.8.8.8" in JSON configuration.
     */
    const os_char *dns_address_2;

    /** Harware address for network adapter. Some embedded systems or network
        adapters (like Wiz5500, I think) or do not have pre configured MAC address,
        so it can be set here. For example "12:A3:CE:87:12:B2"
        Locally administered MAC address ranges safe for testing: x2:xx:xx:xx:xx:xx,
        x6:xx:xx:xx:xx:xx, xA:xx:xx:xx:xx:xx and xE:xx:xx:xx:xx:xx
        If you sell commercially, you should buy these for your product, see
        https://standards.ieee.org. If you are just testing just invent fansom
        MACs from ranges safe for testing: Fill all 'x' characters with
        hexadecimal digit '0'-'9', 'A'-'F'. Even MAC must be unique within
        local network, it is extremely unlikely to have conflict is all 'x'
        values are truely random. Set by "mac":"36:12:64:A4:B4:C4" in JSON
        configuration.
    */
    const os_char *mac;

    /** Disable dynamic host configuration protocol, DHCP for short. If this is
        OS_FALSE this NIC tries to get IP network address from DHCP server in local
        network and static network configuration parameters (host name, up, subnet,
        gateway  and DNS) are ignored. If selected (OS_TRUE), static network
        configuration is used as given. This parameter is used only for microcontrollers.
        Use "dhcp":0 in JSON configuration to disable.
     */
    os_boolean no_dhcp;

    /** OS_TRUE to enable sending UDP multicasts trough this network interface.
        What happends when UDP multicast is sent and multicasts are not enabled
        for any NIC, or are are selected for multiple NICs, is up to operating
        system specific implementation. If interface is defined when opening
        the socked, this parameter is ignored. Use "send_udp":1 in JSON configuration
        to enable.
     */
    os_boolean send_udp_multicasts;

    /** OS_TRUE to receiving UDP multicasts trough this NIC. What happends if
        receiving UDP multicasts is not enabled for any NIC, or is enabled
        for multiple NICs, is up to operating system specific implementation.
        If interface is defined when opening the socked, this parameter is
        ignored. Use "receive_udp":1 in JSON configuration to enable.
     */
    os_boolean receive_udp_multicasts;
}
osalNetworkInterface;

/** Flat structure to for save into persistWifi network name and password.
 */
typedef struct osalWifiNetworkBuf
{
    /** WiFi network name, same as SSID.
     */
    os_char wifi_net_name[OSAL_WIFI_PRM_SZ];

    /** WiFi network password, pre shared key.
     */
    os_char wifi_net_password[OSAL_WIFI_PRM_SZ];
}
osalWifiNetworkBuf;

/** Structure to save Wifi and other basic network configuration as persistent
    block numer OS_PBNR_WIFI. If set, these override
 */
typedef struct osalWifiPersistent
{
    osalWifiNetworkBuf wifi[OSAL_MAX_NRO_WIFI_NETWORKS];

    /* If set, these will overdrive settings elsewhere.
     */
    os_char network_name_overdrive[OSAL_NETWORK_NAME_SZ];
    os_char device_nr_overdrive[OSAL_DEVICE_NR_STR_SZ];
    os_char connect_to_overdrive[OSAL_HOST_BUF_SZ];
}
osalWifiPersistent;

/** Information anout one light house end point.
 */
typedef struct iocLighthouseEndPointInfo
{
    /** Transport, IOC_DEFAULT_TRANSPORT (0) if not initialize, otherwise either
        IOC_TCP_SOCKET (1) or IOC_TLS_SOCKET (2).
     */
    os_int transport;

    /** TCP port number listened by server.
     */
    os_int port_nr;

    /** OS_TRUE for IPv6 or OS_FALSE for IPv4.
     */
    os_boolean is_ipv6;
}
iocLighthouseEndPointInfo;

/** Maximum number of end points to store into info
 */
#define IOC_LIGHTHOUSE_INFO_MAX_END_POINTS 4

/** Information for light house (multicast device discovery) from node configuration.
 */
typedef struct iocLighthouseInfo
{
    /** End point array
     */
    iocLighthouseEndPointInfo epoint[IOC_LIGHTHOUSE_INFO_MAX_END_POINTS];

    /** Number of end points in array.
     */
    os_int n_epoints;
}
iocLighthouseInfo;
