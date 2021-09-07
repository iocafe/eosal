/**

  @file    net/esp32/osal_esp32_wifi_init.cpp
  @brief   ESP32 WiFi network initialization for ESP_IDF framework.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    3.9.2021

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
#ifdef OSAL_ESPIDF_FRAMEWORK
#if (OSAL_SOCKET_SUPPORT & OSAL_NET_INIT_MASK) == OSAL_ESPIDF_WIFI_INIT

#include "esp_pm.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_event.h"

/* Global network adapter and wifi info
 */
#include "extensions/net/common/osal_shared_net_info.h"
static osalSocketGlobal sg;

/** Which NIC index we use for WiFi, for now we always assume that the first NIC is WiFi.
 */
#define OSAL_WIFI_NIC_IX 0

typedef struct
{
    /* Current status of the WiFi connection.
     */
    volatile osalStatus s;
    volatile osalStatus got_ip;

    os_boolean no_dhcp;
}
osalWifiNetworkState;

/* The network settings and state needed for WiFi initialization.
 */
static osalWifiNetworkState wifistate;

/* Forward referred.
 */
static void osal_wifi_event_handler(void* arg, esp_event_base_t event_base,
		int32_t event_id, void* event_data);


/**
****************************************************************************************************

  @brief Initialize Wifi network.
  @anchor osal_socket_initialize

  The osal_socket_initialize() initializes the underlying network/wifi/sockets libraries
  and starts up wifi networking. 

  @param   nic Pointer to array of network interface structures. Current ESP32 implementation
           supports only one network interface.
  @param   n_nics Number of network interfaces in nic array. 1 or more, only first NIC used.
  @param   wifi Pointer to array of WiFi network structures. This contains wifi network 
           name (SSID) and password (pre shared key) pairs. Can be OS_NULL if there is no WiFi.
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
    esp_err_t rval;
    esp_netif_t *sta_netif;

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
    os_memclear(&wifistate, sizeof(wifistate));
    wifistate.s = OSAL_PENDING;
    wifistate.got_ip = OSAL_PENDING;

    /* Initialize the underlying TCP/IP stack.
     */
	rval = esp_netif_init();
    osal_debug_assert(rval == ESP_OK);

    /* Create event loop to pass WiFi related events. ?????? SHOULD THERE BE ONLY ONE PER APP ??????
     */
	rval = esp_event_loop_create_default();
    osal_debug_assert(rval == ESP_OK);

    /* Create default WIFI STA. In case of any init error this API aborts. 
     */
	sta_netif = esp_netif_create_default_wifi_sta();
    osal_debug_assert(sta_netif != 0);

    /* Initialize WiFi. 
     */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    rval = esp_wifi_init(&cfg);    
    osal_debug_assert(rval == ESP_OK);

    /* Add event handlers.
     */
    rval = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
        &osal_wifi_event_handler, NULL);
    osal_debug_assert(rval == ESP_OK);
	rval = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
        &osal_wifi_event_handler, NULL);
    osal_debug_assert(rval == ESP_OK);
#if EXAMPLE_WIFI_RSSI_THRESHOLD
	rval = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_BSS_RSSI_LOW,
				&esp_bss_rssi_low_handler, NULL);
    osal_debug_assert(rval == ESP_OK);
#endif

    /* Do not keep WiFi configuration on flash.
     */
    rval = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    osal_debug_assert(rval == ESP_OK);

    /* Power management off. IMPORTANT, OTHERWISE WIFI WILL CRAWL.
     */
    rval = esp_wifi_set_ps(WIFI_PS_NONE);
    osal_debug_assert(rval == ESP_OK);

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = "lama24",
			.password = "talvi333",
			.rm_enabled =1,
			.btm_enabled =1,
		},
	};

    osal_trace_str("WiFi: ", (os_char*)wifi_config.sta.ssid);
	rval = esp_wifi_set_mode(WIFI_MODE_STA);
    osal_debug_assert(rval == ESP_OK);
	rval = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (rval != ESP_OK) {
        osal_debug_error("esp_wifi_set_config failed");
        return;            
    }
	rval = esp_wifi_start();
    if (rval != ESP_OK) {
        osal_debug_error("esp_wifi_start failed");
        return;            
    }

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
    wifistate.wifi_multi_on = OS_FALSE;
    if (n_wifi > 1)
    {
        wifistate.wifi_multi_on = (wifi[1].wifi_net_name[0] != '\0');
        if (wifistate.wifi_multi_on)
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

    wifistate.no_dhcp = nic[0].no_dhcp;

    /* Set socket library initialized flag, now waiting for wifi initialization. We do not lock
     * the code here to allow IO sequence, etc to proceed even without wifi.
     */
    osal_global->socket_global = &sg;
    osal_global->sockets_shutdown_func = osal_socket_shutdown;
    return;
}


/**
****************************************************************************************************

  @brief Handle WiFi and IP events.
  @anchor osal_wifi_event_handler

  This function gets called when WiFi is connected, disconnected, or the ESP32 receives an IP
  address.

****************************************************************************************************
*/
static void osal_wifi_event_handler(
    void *arg, 
    esp_event_base_t event_base,
    int32_t event_id, 
    void *event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
		esp_wifi_connect();
        osal_trace("WIFI_EVENT_STA_START");
	}
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		wifi_event_sta_disconnected_t *disconn = event_data;
		if (disconn->reason == WIFI_REASON_ROAMING) {
			osal_trace("station roaming, do nothing");
		} 
        else {
            osal_trace("WIFI_EVENT_STA_DISCONNECTED");
            wifistate.s = OSAL_STATUS_FAILED;
            wifistate.got_ip = OSAL_PENDING; 
			esp_wifi_connect();
		}
	} 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) 
    {
        osal_trace("WIFI_EVENT_STA_CONNECTED");

#if EXAMPLE_WIFI_RSSI_THRESHOLD
		if (EXAMPLE_WIFI_RSSI_THRESHOLD) {
			ESP_LOGI(TAG, "setting rssi threshold as %d\n", EXAMPLE_WIFI_RSSI_THRESHOLD);
			esp_wifi_set_rssi_threshold(EXAMPLE_WIFI_RSSI_THRESHOLD);
		}
#endif
        wifistate.s = OSAL_SUCCESS;
        // if (wifistate.no_dhcp) {
        //    osal_error(OSAL_CLEAR_ERROR, eosal_mod, OSAL_STATUS_NO_WIFI, "hi"); 
        //}
	}

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        osal_trace("IP_EVENT_STA_GOT_IP");
        //osal_error(OSAL_CLEAR_ERROR, eosal_mod, OSAL_STATUS_NO_WIFI, "hi"); 
        wifistate.got_ip = OSAL_SUCCESS;
    }
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
           OSAL_STATUS_FAILED No connection, at least for now.

****************************************************************************************************
*/
osalStatus osal_are_sockets_initialized(
    void)
{
    if (wifistate.s == OSAL_SUCCESS) {
        return wifistate.got_ip;
    }

    return wifistate.s; 
}


/**
****************************************************************************************************

  @brief Shut down sockets.
  @anchor osal_socket_shutdown

  The osal_socket_shutdown() shuts down the underlying sockets library.

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
#endif
