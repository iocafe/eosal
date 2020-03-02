/**

  @file    socket/arduino/osal_arduino_wifi.cpp
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
/* Force tracing on for this source file.
 */
/* #define OSAL_TRACE 3 */

#include "eosalx.h"
#if 0
// #if OSAL_SOCKET_SUPPORT & OSAL_ARDUINO_WIFI_INIT_BIT


/** Use WifiMulti to automatically select one from known access points. Define 1 or 0.
 */
#define OSAL_WIFI_MULTI 1

#include <Arduino.h>
#include <WiFi.h>

#if OSAL_WIFI_MULTI
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#endif

#include <esp_pm.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>

/* Two known wifi networks to select from in NIC configuration.
 */
static os_boolean osal_wifi_multi_on = OS_FALSE;


typedef struct
{
    os_char ip_address[OSAL_HOST_BUF_SZ];

    IPAddress
        dns_address,
        dns_address_2,
        gateway_address,
        subnet_mask;

    os_boolean no_dhcp;

    os_char wifi_net_name[OSAL_WIFI_PRM_SZ];
    os_char wifi_net_password[OSAL_WIFI_PRM_SZ];

}
osalArduinoNetParams;

/* The osal_socket_initialize() stores application's network setting here. The values in
   are then used and changed by initialization to reflect initialized state.
 */
static osalArduinoNetParams osal_wifi_nic;

/* Socket library initialized flag.
 */
os_boolean osal_sockets_initialized = OS_FALSE;

/* WiFi connected flag.
 */
os_boolean osal_wifi_connected = OS_FALSE;

enum {
    OSAL_WIFI_INIT_STEP1,
    OSAL_WIFI_INIT_STEP2,
    OSAL_WIFI_INIT_STEP3
}
osal_wifi_init_step;

static os_boolean osal_wifi_init_failed_once;
static os_boolean osal_wifi_init_failed_now;
static os_boolean osal_wifi_was_connected;
static os_timer osal_wifi_step_timer;
static os_timer osal_wifi_boot_timer;


// void osal_socket_on_wifi_connect(void);
// void osal_socket_on_wifi_disconnect(void);

// static void osal_socket_start_wifi_init(void);



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
    if (nic == OS_NULL && n_nics < 1)
    {
        osal_debug_error("osal_socket_initialize(): No NIC configuration");
    }

#if OSAL_WIFI_MULTI
    /* Use WiFiMulti if we have second access point.
     */
    osal_wifi_multi_on = OS_FALSE;
    if (n_wifi > 1)
    {
        osal_wifi_multi_on = (wifi[1].wifi_net_name[0] != '\0');
        if (osal_wifi_multi_on)
        {
            wifiMulti.addAP(wifi[0].wifi_net_name, wifi[0].wifi_net_password);
            wifiMulti.addAP(wifi[1].wifi_net_name, wifi[1].wifi_net_password);
        }
    }
#endif

    os_memclear(osal_socket, sizeof(osal_socket));
    os_memclear(osal_client_state, sizeof(osal_client_state));
    os_memclear(osal_server_state, sizeof(osal_server_state));

    os_strncpy(osal_wifi_nic.ip_address, nic[0].ip_address, OSAL_HOST_BUF_SZ);
    osal_arduino_ip_from_str(osal_wifi_nic.dns_address, nic[0].dns_address);
    osal_arduino_ip_from_str(osal_wifi_nic.dns_address_2, nic[0].dns_address_2);
    osal_arduino_ip_from_str(osal_wifi_nic.gateway_address, nic[0].gateway_address);
    osal_arduino_ip_from_str(osal_wifi_nic.subnet_mask, nic[0].subnet_mask);
    osal_wifi_nic.no_dhcp = nic[0].no_dhcp;
    os_strncpy(osal_wifi_nic.wifi_net_name, wifi[0].wifi_net_name, OSAL_WIFI_PRM_SZ);
    os_strncpy(osal_wifi_nic.wifi_net_password, wifi[0].wifi_net_password,OSAL_WIFI_PRM_SZ);

    /* Start wifi initialization.
     */
    osal_socket_start_wifi_init();

    /* Set socket library initialized flag.
     */
    osal_sockets_initialized = OS_TRUE;
}


/**
****************************************************************************************************

  @brief Start Wifi initialisation from beginning.
  @anchor osal_socket_start_wifi_init

  The osal_socket_start_wifi_init() function starts wifi initialization. The
  initialization is continued by repeatedly called osal_are_sockets_initialized() function.

  @return  None.

****************************************************************************************************
*/
static void osal_socket_start_wifi_init(void)
{
    /* Start the WiFi. Do not wait for the results here, we wish to allow IO to run even
       without WiFi network.
     */
    osal_trace("Commecting to Wifi network");

    /* Set socket library initialized flag, now waiting for wifi initialization. We do not lock
     * the code here to allow IO sequence, etc to proceed even without wifi.
     */
    osal_wifi_init_step = OSAL_WIFI_INIT_STEP1;
    osal_wifi_init_failed_once = OS_FALSE;

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

    if (!osal_sockets_initialized) return OSAL_STATUS_FAILED;

    s = osal_wifi_init_failed_once
        ? OSAL_STATUS_FAILED : OSAL_PENDING;

    switch (osal_wifi_init_step)
    {
        case OSAL_WIFI_INIT_STEP1:
            /* The following four lines are silly stuff to reset
               the ESP32 wifi after soft reboot. I assume that this will be fixed and
               become unnecessary at some point.
             */
#if OSAL_SOCKET_SUPPORT==OSAL_ARDUINO_WIFI_ESP32
            WiFi.mode(WIFI_OFF);
            WiFi.mode(WIFI_STA);
            WiFi.disconnect();
            WiFi.getMode();
            WiFi.status();
#endif

            osal_wifi_connected = osal_wifi_was_connected = OS_FALSE;
            osal_wifi_init_failed_now = OS_FALSE;
            os_get_timer(&osal_wifi_step_timer);
            osal_wifi_boot_timer = osal_wifi_step_timer;
            osal_wifi_init_step = OSAL_WIFI_INIT_STEP2;

            esp_wifi_set_ps(WIFI_PS_NONE);  // XXXXXXXXXXXXXXXXXXXXXX REALLY REALLY IMPORTANT, OTHERWISE WIFI WILL CRAWL
            break;

        case OSAL_WIFI_INIT_STEP2:
            if (os_elapsed(&osal_wifi_step_timer, 100))
            {
                /* Start the WiFi.
                 */
                if (!osal_wifi_multi_on)
                {
                    /* Initialize using static configuration.
                     */
                    if (osal_wifi_nic.no_dhcp)
                    {
                        /* Some default network parameters.
                         */
                        IPAddress
                            ip_address(192, 168, 1, 195);

                        osal_arduino_ip_from_str(ip_address, osal_wifi_nic.ip_address);

#if OSAL_SOCKET_SUPPORT==OSAL_ARDUINO_WIFI_ESP32
                        /* Warning: ESP does not follow same argument order as arduino,
                           one below is for ESP32.
                         */
                        if (!WiFi.config(ip_address, osal_wifi_nic.gateway_address,
                            osal_wifi_nic.subnet_mask,
                            osal_wifi_nic.dns_address, osal_wifi_nic.dns_address_2))
                        {
                            osal_debug_error("Static IP configuration failed");
                        }
#else
                        if (!WiFi.config(ip_address, osal_wifi_nic.dns_address,
                            osal_wifi_nic.gateway_address, osal_wifi_nic.subnet_mask)
                        {
                            osal_debug_error("Static IP configuration failed");
                        }
#endif
                    }

                    WiFi.begin(osal_wifi_nic.wifi_net_name, osal_wifi_nic.wifi_net_password);
                }

                os_get_timer(&osal_wifi_step_timer);
                osal_wifi_init_step = OSAL_WIFI_INIT_STEP3;
                osal_trace("Connecting wifi");
            }
            break;

        case OSAL_WIFI_INIT_STEP3:
#if OSAL_WIFI_MULTI
            if (!osal_wifi_multi_on)
            {
                osal_wifi_connected = (os_boolean) (WiFi.status() == WL_CONNECTED);
            }
            else
            {
                osal_wifi_connected = (os_boolean) (wifiMulti.run() == WL_CONNECTED);
            }
#else
            osal_wifi_connected = (os_boolean) (WiFi.status() == WL_CONNECTED);
#endif

            /* If no change in connection status:
               - If we are connected or connection has never failed (boot), or
                 not connected, return appropriate status code. If not con
             */
            if (osal_wifi_connected == osal_wifi_was_connected)
            {
                if (osal_wifi_connected)
                {
                    s = OSAL_SUCCESS;
                    break;
                }

                if (osal_wifi_init_failed_now)
                {
                    s = OSAL_STATUS_FAILED;
                }

                else
                {
                    if (os_elapsed(&osal_wifi_step_timer, 8000))
                    {
                        osal_wifi_init_failed_now = OS_TRUE;
                        osal_wifi_init_failed_once = OS_TRUE;
                        osal_trace("Unable to connect Wifi");
                    }

                    s = osal_wifi_init_failed_once
                        ? OSAL_STATUS_FAILED : OSAL_PENDING;
                }

                break;
            }

            /* Save to detect connection state changes.
             */
            osal_wifi_was_connected = osal_wifi_connected;

            /* If this is connect
             */
            if (osal_wifi_connected)
            {
                s = OSAL_SUCCESS;
                osal_trace_str("Wifi network connected: ", WiFi.SSID().c_str());
                osal_socket_on_wifi_connect();

#if OSAL_TRACE
                IPAddress ip = WiFi.localIP();
                String strip = DisplayAddress(ip);
                osal_trace(strip.c_str());
#endif
            }

            /* Otherwise this is disconnect.
             */
            else
            {
                osal_wifi_init_step = OSAL_WIFI_INIT_STEP1;
                osal_trace("Wifi network disconnected");
                osal_socket_on_wifi_disconnect();
                s = OSAL_STATUS_FAILED;
            }

            break;
    }

    return s;
}


/**
****************************************************************************************************

  @brief Called when WiFi network is connected.
  @anchor osal_socket_on_wifi_connect

  The osal_socket_on_wifi_connect() function...
  @return  None.

****************************************************************************************************
*/
void osal_socket_on_wifi_connect(
    void)
{
    osalSocket *mysocket;
    os_int i, ix;

    for (i = 0; i<OSAL_MAX_SOCKETS; i++)
    {
        mysocket = osal_socket + i;
        ix = mysocket->index;
        switch (mysocket->use)
        {
            case OSAL_SOCKET_UNUSED:
                break;

            case OSAL_SOCKET_CLIENT:
                if (osal_client_state[ix] != OSAL_PREPARED_STATE) break;
                if (osal_socket_really_connect(mysocket))
                {
                    osal_client_state[ix] = OSAL_FAILED_STATE;
                }
                break;

            case OSAL_SOCKET_SERVER:
                if (osal_server_state[ix] != OSAL_PREPARED_STATE &&
                    osal_server_state[ix] != OSAL_FAILED_STATE)
                {
                    break;
                }
                if (osal_socket_really_listen(mysocket))
                {
                    osal_server_state[ix] = OSAL_FAILED_STATE;
                }
                break;

            case OSAL_SOCKET_UDP:
                break;
        }
    }
}


/**
****************************************************************************************************

  @brief Called when connected WiFi network is disconnected.
  @anchor osal_socket_on_wifi_disconnect

  The osal_socket_on_wifi_disconnect() function...
  @return  None.

****************************************************************************************************
*/
void osal_socket_on_wifi_disconnect(
    void)
{
    osalSocket *mysocket;
    os_int i, ix;

    for (i = 0; i<OSAL_MAX_SOCKETS; i++)
    {
        mysocket = osal_socket + i;
        ix = mysocket->index;
        switch (mysocket->use)
        {
            case OSAL_SOCKET_UNUSED:
                break;

            case OSAL_SOCKET_CLIENT:
                if (osal_client_state[ix] != OSAL_RUNNING_STATE) break;
                osal_client_state[ix] = OSAL_FAILED_STATE;
                break;

            case OSAL_SOCKET_SERVER:
                if (osal_server_state[ix] != OSAL_RUNNING_STATE) break;
                osal_server[ix].stop();
                osal_server_state[ix] = OSAL_FAILED_STATE;
                mysocket->sockindex = 0;
                break;

            case OSAL_SOCKET_UDP:
                break;
        }
    }
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
    if (osal_sockets_initialized)
    {
        WiFi.disconnect();
        osal_sockets_initialized = OS_FALSE;
    }
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
