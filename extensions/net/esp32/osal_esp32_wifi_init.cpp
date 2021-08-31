/**

  @file    net/esp32/osal_esp32_wifi_init.cpp
  @brief   OSAL Ardyino WiFi network initialization.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    1.3.2020

  WiFi connectivity. Wifi network initialization.

  Features:
  - WiFiMulti to allows automatic switching between two known wifi networks. Notice that if
    two wifi networks are specified in NIC connfiguration, then static network configuration
    cannot be used and DHCP will be enabled.

  Notes:
  - Wifi.config() function in ESP does not follow same argument order as arduino. This
    can create problem if using static IP address.
  - Static WiFi IP address doesn't work for ESP32. This seems to be a bug in espressif Arduino
    support (replacing success check with 15 sec delay will patch it). Wait for espressif
    updates, ESP32 is still quite new.
  - esp_wifi_set_ps(WIFI_PS_NONE);  // XXXXXXXXXXXXXXXXXXXXXX REALLY REALLY IMPORTANT, OTHERWISE WIFI WILL CRAWL

  MISSING - TO BE DONE
  - DNS to resolve host names

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#ifdef OSAL_ESP32
#if (OSAL_SOCKET_SUPPORT & OSAL_NET_INIT_MASK) == OSAL_ARDUINO_WIFI_INIT

/** Do we include code to automatically select one from known access points. Define 1 or 0.
 */
#define OSAL_SUPPORT_WIFI_MULTI 1

#include <Arduino.h>
#include <WiFi.h>

#if OSAL_SUPPORT_WIFI_MULTI
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#endif

#include <esp_pm.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>

/* Global network adapter and wifi info
 */
extern "C" {
#include "extensions/net/common/osal_shared_net_info.h"
}
static osalSocketGlobal sg;

/** Which NIC index we use for WiFi, for now we always assume that the first NIC is WiFi.
 */
#define OSAL_WIFI_NIC_IX 0

typedef enum {
    OSAL_WIFI_INIT_STEP1,
    OSAL_WIFI_INIT_STEP2,
    OSAL_WIFI_INIT_STEP3
}
osalArduinoWifiInitStep;


typedef struct
{
    os_char ip_address[OSAL_HOST_BUF_SZ];

    IPAddress
        dns_address,
        dns_address_2,
        gateway_address,
        subnet_mask;

    os_boolean no_dhcp;

    /* Two known wifi networks to select from in NIC configuration.
     */
    os_boolean wifi_multi_on;

    /* WiFi connected flag.
     */
    os_boolean network_connected;

    osalArduinoWifiInitStep wifi_init_step;

    os_boolean wifi_init_failed_once;
    os_boolean wifi_init_failed_now;
    os_boolean wifi_was_connected;
    os_timer wifi_step_timer;
    os_timer wifi_boot_timer;
}
osalArduinoNetStruct;


/* The network settings and state needed for WiFi initialization.
 */
static osalArduinoNetStruct ans;



/**
****************************************************************************************************

  @brief Convert string to binary IP address.
  @anchor osal_arduino_ip_from_str

  The osal_arduino_ip_from_str() converts string representation of IP address to binary.
  If the function fails, binary IP address is left unchanged.

  @param   ip Pointer to Arduino IP address to set.
  @param   str Input, IP address as string.
  @return  None.

****************************************************************************************************
*/
static void osal_arduino_ip_from_str(
    IPAddress& ip,
    const os_char *str)
{
    os_uchar buf[4];
    os_short i;

    if (osal_ip_from_str(buf, sizeof(buf), str) == OSAL_SUCCESS)
    {
        for (i = 0; i < sizeof(buf); i++) ip[i] = buf[i];
    }
}


static String DisplayAddress(IPAddress address)
{
 return String(address[0]) + "." +
        String(address[1]) + "." +
        String(address[2]) + "." +
        String(address[3]);
}


/**
****************************************************************************************************

  @brief Initialize sockets LWIP/WizNet.
  @anchor osal_socket_initialize

  The osal_socket_initialize() initializes the underlying sockets library. This used either DHCP,
  or statical configuration parameters.

  Network interface configuration must be given to osal_socket_initialize() as when using wifi,
  because wifi SSID (wifi net name) and password are required to connect.

  @param   nic Pointer to array of network interface structures. A network interface is needed,
           and The arduino Wifi implemetation
           supports only one network interface.
  @param   n_nics Number of network interfaces in nic array. 1 or more, only first NIC used.
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
    os_int i;
    osalNetworkInterface defaultnic;

    if (nic == OS_NULL && n_nics < 1)
    {
        osal_debug_error("osal_socket_initialize(): No NIC configuration");
        os_memclear(&defaultnic, sizeof(defaultnic));
        nic = &defaultnic;
        n_nics = 1;
    }

    /* If socket library is already initialized, do nothing.
     */
    if (osal_global->socket_global) return;
    os_memclear(&sg, sizeof(sg));
    os_memclear(&ans, sizeof(ans));

    /* Do not keep wifi configuration on flash.
     */
    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    /** Copy NIC settings.
     */
    if (nic) for (i = 0; i < n_nics; i++)
    {
        os_strncpy(sg.nic[sg.n_nics].ip_address, nic[i].ip_address, OSAL_IPADDR_SZ);
        sg.nic[sg.n_nics].receive_udp_multicasts = nic[i].receive_udp_multicasts;
        sg.nic[sg.n_nics].send_udp_multicasts = nic[i].send_udp_multicasts;
        if (++(sg.n_nics) >= OSAL_MAX_NRO_NICS) break;
    }

#if OSAL_SUPPORT_WIFI_MULTI
    /* Use WiFiMulti if we have second access point.
     */
    ans.wifi_multi_on = OS_FALSE;
    if (n_wifi > 1)
    {
        ans.wifi_multi_on = (wifi[1].wifi_net_name[0] != '\0');
        if (ans.wifi_multi_on)
        {
            wifiMulti.addAP(wifi[0].wifi_net_name, wifi[0].wifi_net_password);
            wifiMulti.addAP(wifi[1].wifi_net_name, wifi[1].wifi_net_password);
        }
    }
#endif

    for (i = 0; i < n_wifi; i++)
    {
        osal_set_network_state_str(OSAL_NS_WIFI_NETWORK_NAME, i, wifi[i].wifi_net_name);
        osal_set_network_state_str(OSAL_NS_WIFI_PASSWORD, i, wifi[i].wifi_net_password);
    }

    os_strncpy(ans.ip_address, nic[0].ip_address, OSAL_HOST_BUF_SZ);
    osal_arduino_ip_from_str(ans.dns_address, nic[0].dns_address);
    osal_arduino_ip_from_str(ans.dns_address_2, nic[0].dns_address_2);
    osal_arduino_ip_from_str(ans.gateway_address, nic[0].gateway_address);
    osal_arduino_ip_from_str(ans.subnet_mask, nic[0].subnet_mask);
    ans.no_dhcp = nic[0].no_dhcp;

    /* Start the WiFi. Do not wait for the results here, we wish to allow IO to run even
       without WiFi network.
     */
    osal_trace("Commecting to Wifi network");

    /* Set socket library initialized flag, now waiting for wifi initialization. We do not lock
     * the code here to allow IO sequence, etc to proceed even without wifi.
     */
    ans.wifi_init_step = OSAL_WIFI_INIT_STEP1;
    ans.wifi_init_failed_once = OS_FALSE;
    osal_global->socket_global = &sg;
    osal_global->sockets_shutdown_func = osal_socket_shutdown;

    /* Call wifi init once to move once to start it.
     */
    osal_are_sockets_initialized();
}


/**
****************************************************************************************************

  @brief Check if WiFi network is connected.
  @anchor osal_are_sockets_initialized

  Called to check if WiFi initialization has been completed and if so, the function initializes
  has been initialized and connected. Once connection is detected,
  the LWIP library is initialized.

  @return  OSAL_SUCCESS if we are connected to a wifi network.
           OSAL_PENDING If currently connecting and have not never failed to connect so far.
           OSAL_STATUS_FALED No connection, at least for now.

****************************************************************************************************
*/
osalStatus osal_are_sockets_initialized(
    void)
{
    osalStatus s;
    os_char wifi_net_name[OSAL_WIFI_PRM_SZ];
    os_char wifi_net_password[OSAL_WIFI_PRM_SZ];

    if (osal_global->socket_global == OS_NULL) return OSAL_STATUS_FAILED;

// SYNCHRONIZE XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

    s = ans.wifi_init_failed_once
        ? OSAL_STATUS_FAILED : OSAL_PENDING;

    switch (ans.wifi_init_step)
    {
        case OSAL_WIFI_INIT_STEP1:
            osal_set_network_state_int(OSAL_NS_NETWORK_CONNECTED, 0, OS_FALSE);
            osal_set_network_state_int(OSAL_NS_NETWORK_USED, 0, OS_TRUE);

            /* The following four lines are silly stuff to reset
               the ESP32 wifi after soft reboot. I assume that this will be fixed and
               become unnecessary at some point.
             */
            WiFi.mode(WIFI_OFF);
            WiFi.mode(WIFI_STA);
            WiFi.disconnect();
            WiFi.getMode();
            WiFi.status();

            ans.network_connected = ans.wifi_was_connected = OS_FALSE;
            ans.wifi_init_failed_now = OS_FALSE;
            os_get_timer(&ans.wifi_step_timer);
            ans.wifi_boot_timer = ans.wifi_step_timer;

            /* Power management off. REALLY REALLY IMPORTANT, OTHERWISE WIFI WILL CRAWL.
             */
            esp_wifi_set_ps(WIFI_PS_NONE);

            ans.wifi_init_step = OSAL_WIFI_INIT_STEP2;
            break;

        case OSAL_WIFI_INIT_STEP2:
            if (os_has_elapsed(&ans.wifi_step_timer, 100))
            {
                /* Start the WiFi.
                 */
                if (!ans.wifi_multi_on)
                {
                    /* Initialize using static configuration.
                     */
                    if (ans.no_dhcp)
                    {
                        /* Some default network parameters.
                         */
                        IPAddress ip_address(192, 168, 1, 195);
                        osal_arduino_ip_from_str(ip_address, ans.ip_address);

                        /* Warning: ESP does not follow same argument order as arduino,
                           one below is for ESP32.
                         */
                        if (!WiFi.config(ip_address, ans.gateway_address,
                            ans.subnet_mask,
                            ans.dns_address, ans.dns_address_2))
                        {
                            osal_debug_error("Static IP configuration failed");
                        }
                    }

                    osal_get_network_state_str(OSAL_NS_WIFI_NETWORK_NAME, 0,
                        wifi_net_name, sizeof(wifi_net_name));
                    osal_get_network_state_str(OSAL_NS_WIFI_PASSWORD, 0,
                        wifi_net_password, sizeof(wifi_net_password));
                    WiFi.begin(wifi_net_name, wifi_net_password);
                }

                os_get_timer(&ans.wifi_step_timer);
                ans.wifi_init_step = OSAL_WIFI_INIT_STEP3;
                osal_trace("Connecting wifi");
            }
            break;

        case OSAL_WIFI_INIT_STEP3:
#if OSAL_SUPPORT_WIFI_MULTI
            if (!ans.wifi_multi_on)
            {
                ans.network_connected = (os_boolean) (WiFi.status() == WL_CONNECTED);
            }
            else
            {
                ans.network_connected = (os_boolean) (wifiMulti.run() == WL_CONNECTED);
            }
#else
            ans.network_connected = (os_boolean) (WiFi.status() == WL_CONNECTED);
#endif

            /* If no change in connection status:
               - If we are connected or connection has never failed (boot), or
                 not connected, return appropriate status code. If not con
             */
            if (ans.network_connected == ans.wifi_was_connected)
            {
                if (ans.network_connected)
                {
                    s = OSAL_SUCCESS;
                    break;
                }

                if (ans.wifi_init_failed_now)
                {
                    s = OSAL_STATUS_FAILED;
                }

                else
                {
                    if (os_has_elapsed(&ans.wifi_step_timer, 10000))
                    {
                        ans.wifi_init_failed_now = OS_TRUE;
                        ans.wifi_init_failed_once = OS_TRUE;
                        osal_trace("Unable to connect Wifi");
                        osal_error(OSAL_ERROR, eosal_mod, OSAL_STATUS_NO_WIFI, OS_NULL);
                    }

                    s = ans.wifi_init_failed_once
                        ? OSAL_STATUS_FAILED : OSAL_PENDING;
                }

                break;
            }

            /* Save to detect connection state changes.
             */
            ans.wifi_was_connected = ans.network_connected;

            /* If this is connect
             */
            if (ans.network_connected)
            {
                s = OSAL_SUCCESS;
                osal_trace_str("Wifi network connected: ", WiFi.SSID().c_str());

                /* SETUP TO RECEIVE multicasts from this IP address.
                 */
                IPAddress ip = WiFi.localIP();
                String addrstr = DisplayAddress(ip);
                const os_char *p = addrstr.c_str();
                os_strncpy(sg.nic[OSAL_WIFI_NIC_IX].ip_address, p, OSAL_IPADDR_SZ);
                osal_error(OSAL_CLEAR_ERROR, eosal_mod, OSAL_STATUS_NO_WIFI, p);
                osal_set_network_state_int(OSAL_NS_NETWORK_CONNECTED, 0, OS_TRUE);
#if OSAL_TRACE
                osal_trace(addrstr.c_str());
#endif
            }

            /* Otherwise this is disconnect.
             */
            else
            {
//                ans.wifi_init_step = OSAL_WIFI_INIT_STEP1;   PEKKA TEST NOT TO REINITIALIZE
                osal_trace("Wifi network disconnected");
                s = OSAL_STATUS_FAILED;
            }

            break;
    }

    return s;
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
    osal_global->socket_global = OS_NULL;
}


#if OSAL_SOCKET_MAINTAIN_NEEDED
/**
****************************************************************************************************

  @brief Keep the sockets library alive.
  @anchor osal_socket_maintain

  The osal_socket_maintain() function is not needed for Arduino WiFi, empty function is here
  just to allow build if OSAL_SOCKET_MAINTAIN_NEEDED is on.

  @return  None.

****************************************************************************************************
*/
void osal_socket_maintain(
    void)
{
    #warning Unnecessary OSAL_SOCKET_MAINTAIN_NEEDED=1 define, remove to save a few bytes
}
#endif

#endif
#endif
