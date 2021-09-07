/**

  @file    net/esp32/osal_esp32_wifi_init.cpp
  @brief   ESP32 WiFi network initialization for ESP_IDF framework.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    3.9.2021

  TODO, WHAT COULD BE DONE HERE
  - DNS to resolve host names.
  - Static IP address support.
  - Multi to allows automatic switching between two known wifi networks. Optionally roaming.
  - ESP32 long distance protocol.
  - AP mode or combined AP/STA mode.

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

    /* Previous status value not to repeat unchanged status.
     */
    osalStatus prev_s;

    /* Flag indicating that static address is used instead of DHCP.
     */
    os_boolean no_dhcp;
}
osalWifiNetworkState;

/* The network settings and state needed for WiFi initialization.
 */
static osalWifiNetworkState wifistate;

/* Forward referred.
 */
static void osal_wifi_event_handler(
    void *arg, 
    esp_event_base_t event_base,
    int32_t event_id, 
    void *event_data);

static void osal_ip_event_handler(
    void *arg, 
    esp_event_base_t event_base,
    int32_t event_id, 
    void *event_data);

static void osal_report_network_state(void);


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
	wifi_config_t wifi_config;

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
    wifistate.prev_s = OSAL_PENDING;   

    /* Initialize the underlying TCP/IP stack.
     */
	rval = esp_netif_init();
    osal_debug_assert(rval == ESP_OK);

    /* Create event loop to pass WiFi related events.
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
	rval = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, 
        &osal_ip_event_handler, NULL);
    osal_debug_assert(rval == ESP_OK);

    /* Do not keep WiFi configuration on flash.
     */
    rval = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    osal_debug_assert(rval == ESP_OK);

    /* Power management off. IMPORTANT, OTHERWISE WIFI WILL CRAWL.
     */
    rval = esp_wifi_set_ps(WIFI_PS_NONE);
    osal_debug_assert(rval == ESP_OK);

    /* This flag is needed to select static IP, not implemented.
     */
    wifistate.no_dhcp = nic[0].no_dhcp;

    /* Configure and start WiFi.
     */
    os_memclear(&wifi_config, sizeof(wifi_config));
    wifi_config.sta.rm_enabled = 1;
    wifi_config.sta.btm_enabled = 1;
    os_strncpy((os_char*)wifi_config.sta.ssid, wifi[0].wifi_net_name,
        sizeof(wifi_config.sta.ssid));
    os_strncpy((os_char*)wifi_config.sta.password, wifi[0].wifi_net_password,
        sizeof(wifi_config.sta.password));
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

    /* Set network state.
     */
    for (i = 0; i < n_wifi; i++)
    {
        osal_set_network_state_str(OSAL_NS_WIFI_NETWORK_NAME, i, wifi[i].wifi_net_name);
        osal_set_network_state_str(OSAL_NS_WIFI_PASSWORD, i, wifi[i].wifi_net_password);
    }
    osal_set_network_state_int(OSAL_NS_NETWORK_CONNECTED, 0, OS_FALSE);
    osal_set_network_state_int(OSAL_NS_NETWORK_USED, 0, OS_TRUE);

    /* Set socket library initialized flag, now waiting for wifi initialization. We do not lock
     * the code here to allow IO sequence, etc to proceed even without wifi.
     */
    osal_global->socket_global = &sg;
    osal_global->sockets_shutdown_func = osal_socket_shutdown;
    return;
}


/**
****************************************************************************************************

  @brief Handle WiFi events.
  @anchor osal_wifi_event_handler

  This function gets called when WiFi is connected or disconnected, etc.

****************************************************************************************************
*/
static void osal_wifi_event_handler(
    void *arg, 
    esp_event_base_t event_base,
    int32_t event_id, 
    void *event_data)
{
    wifi_event_sta_disconnected_t *disconn;

    osal_debug_assert(event_base == WIFI_EVENT);

    switch (event_id) 
    {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            osal_trace("WIFI_EVENT_STA_START");
            break;

        case WIFI_EVENT_STA_CONNECTED: 
            osal_trace("WIFI_EVENT_STA_CONNECTED");
            wifistate.s = OSAL_SUCCESS;
            osal_report_network_state();
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            disconn = event_data;
            if (disconn->reason == WIFI_REASON_ROAMING) {
                osal_trace("station roaming, do nothing");
            } 
            else {
                osal_trace("WIFI_EVENT_STA_DISCONNECTED");
                wifistate.s = OSAL_STATUS_FAILED;
                wifistate.got_ip = OSAL_PENDING; 
                osal_report_network_state();
                esp_wifi_connect();
            }
            break;

        default:
            break;                
    }
}


/**
****************************************************************************************************

  @brief Handle IP events.
  @anchor osal_ip_event_handler

  This function gets called when the ESP32 receives or loses an IP address.

****************************************************************************************************
*/
static void osal_ip_event_handler(
    void *arg, 
    esp_event_base_t event_base,
    int32_t event_id, 
    void *event_data)
{
    osal_debug_assert(event_base == IP_EVENT);

    switch (event_id) 
    {
        case IP_EVENT_STA_GOT_IP:
            osal_trace("IP_EVENT_STA_GOT_IP");
            wifistate.got_ip = OSAL_SUCCESS;
            osal_report_network_state();
            break;

        case IP_EVENT_STA_LOST_IP:
            osal_trace("IP_EVENT_STA_LOST_IP");
            wifistate.got_ip = OSAL_PENDING;
            osal_report_network_state();
            break;

        default:
            break;
    }
}


/**
****************************************************************************************************

  @brief Report network state changed.
  @anchor osal_ip_event_handler

  Reports errors and maintains network state. This information is used by morse code LED 
  and other board status indicators alive.

****************************************************************************************************
*/
static void osal_report_network_state(void)
{
    osalStatus s;

    s = osal_are_sockets_initialized();
    if (s != wifistate.prev_s) {
        wifistate.prev_s = s;
        
        osal_set_network_state_int(OSAL_NS_NETWORK_CONNECTED, 0, s == OSAL_SUCCESS);
        osal_error(s == OSAL_SUCCESS ? OSAL_CLEAR_ERROR : OSAL_ERROR, 
            eosal_mod, OSAL_STATUS_NO_WIFI, OS_NULL); 
    }
}

/**
****************************************************************************************************

  @brief Check if WiFi network is connected.
  @anchor osal_are_sockets_initialized

  Called to check if WiFi initialization has been completed and if so, the function initializes
  has been initialized and connected. 

  @return  OSAL_SUCCESS if we are connected to a wifi network.
           OSAL_PENDING If currently connecting and have not failed to connect so far.
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

  The osal_socket_shutdown() shuts down the underlying sockets library. For ESP32 this doesn't
  do much anything.

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

  The osal_socket_maintain() function is not needed for ESP32, empty function is here
  just to allow build if OSAL_SOCKET_MAINTAIN_NEEDED define is on.

****************************************************************************************************
*/
void osal_socket_maintain(
    void)
{
    #warning Unnecessary OSAL_SOCKET_MAINTAIN_NEEDED=1 define, set to 0 to save a few bytes.
}
#endif

#endif
#endif
#endif
