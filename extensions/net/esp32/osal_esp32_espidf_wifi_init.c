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

/** Do we include code to automatically select one from known access points. Define 1 or 0.
 */
#define OSAL_SUPPORT_WIFI_MULTI 0


#if OSAL_SUPPORT_WIFI_MULTI
// #include <WiFiMulti.h>
// WiFiMulti wifiMulti;
#endif

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

/* typedef enum {
    OSAL_WIFI_INIT_STEP1,
    OSAL_WIFI_INIT_STEP2,
    OSAL_WIFI_INIT_STEP3
}
osalArduinoWifiInitStep;
*/


typedef struct
{
    /* FreeRTOS event group to signal when we are connected & ready to make 
       a request */
    // EventGroupHandle_t wifi_event_group;

    esp_netif_t *sta_netif;

    //os_char ip_address[OSAL_HOST_BUF_SZ];

/*     IPAddress
        dns_address,
        dns_address_2,
        gateway_address,
        subnet_mask; */

    os_boolean no_dhcp;

    /* Two known wifi networks to select from in NIC configuration.
     */
    os_boolean wifi_multi_on;

    /* WiFi connected flag.
     */
    os_boolean network_connected;

    // osalArduinoWifiInitStep  wifi_init_step;

    // os_boolean wifi_init_failed_once;
    // os_boolean wifi_init_failed_now;
    // os_boolean wifi_was_connected;
    // os_timer wifi_step_timer;
    // os_timer wifi_boot_timer;
}
osalWifiNetworkState;


/* The network settings and state needed for WiFi initialization.
 */
static osalWifiNetworkState wifistate;



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
/* static void osal_arduino_ip_from_str(
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
} */


/**
****************************************************************************************************

  @brief Initialize Wifi network.
  @anchor osal_socket_initialize

  The osal_socket_initialize() initializes the underlying network/wifi/sockets libraries. 

  The function saves network interface configuration given as parameter, especially wifi 
  SSID (wifi net name).

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

    /* Initialize the underlying TCP/IP stack.
     */
	rval = esp_netif_init();
    if (rval != ESP_OK) {
        osal_debug_error("esp_netif_init failed");
        return;            
    }

    /* Create event loop to pass WiFi related events. ?????? SHOULD THERE BE ONLY ONE PER APP ??????
     */
	// wifistate.wifi_event_group = xEventGroupCreate();
	rval = esp_event_loop_create_default();
    if (rval != ESP_OK) {
        osal_debug_error("esp_event_loop_create_default failed");
        return;            
    }

    /* Create default WIFI STA. In case of any init error this API aborts. 
     */
	wifistate.sta_netif = esp_netif_create_default_wifi_sta();
    if (wifistate.sta_netif == 0) {
        osal_debug_error("esp_netif_create_default_wifi_sta failed");
        return;            
    }

    /* Initialize WiFi. 
     */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    rval = esp_wifi_init(&cfg);    
    if (rval != ESP_OK) {
        osal_debug_error("esp_wifi_init failed");
        return;            
    }

    /* Add event handlers.
     */
    // rval = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
	// rval = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);
#if EXAMPLE_WIFI_RSSI_THRESHOLD
	rval = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_BSS_RSSI_LOW,
				&esp_bss_rssi_low_handler, NULL);
#endif

    /* Do not keep WiFi configuration on flash.
     */
    rval = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    osal_debug_assert(rval == ESP_OK);

    /* Power management off. REALLY REALLY IMPORTANT, OTHERWISE WIFI WILL CRAWL.
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
    if (rval != ESP_OK) {
        osal_debug_error("esp_wifi_set_mode failed");
        return;            
    }
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

    /* os_strncpy(wifistate.ip_address, nic[0].ip_address, OSAL_HOST_BUF_SZ);
    osal_arduino_ip_from_str(wifistate.dns_address, nic[0].dns_address);
    osal_arduino_ip_from_str(wifistate.dns_address_2, nic[0].dns_address_2);
    osal_arduino_ip_from_str(wifistate.gateway_address, nic[0].gateway_address);
    osal_arduino_ip_from_str(wifistate.subnet_mask, nic[0].subnet_mask); */
    wifistate.no_dhcp = nic[0].no_dhcp;

    /* Start the WiFi. Do not wait for the results here, we wish to allow IO to run even
       without WiFi network.
     */
    // osal_trace("Commecting to Wifi network");

    /* Set socket library initialized flag, now waiting for wifi initialization. We do not lock
     * the code here to allow IO sequence, etc to proceed even without wifi.
     */
    // wifistate.wifi_init_step = OSAL_WIFI_INIT_STEP1;
    // wifistate.wifi_init_failed_once = OS_FALSE;
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
           OSAL_STATUS_FAILED No connection, at least for now.

****************************************************************************************************
*/
osalStatus osal_are_sockets_initialized(
    void)
{
    osalStatus s;

    return OSAL_STATUS_FAILED;
#if 0
    os_char wifi_net_name[OSAL_WIFI_PRM_SZ];
    os_char wifi_net_password[OSAL_WIFI_PRM_SZ];

    if (osal_global->socket_global == OS_NULL) return OSAL_STATUS_FAILED;

    s = wifistate.wifi_init_failed_once
        ? OSAL_STATUS_FAILED : OSAL_PENDING;

    switch (wifistate.wifi_init_step)
    {
        case OSAL_WIFI_INIT_STEP1:
            osal_set_network_state_int(OSAL_NS_NETWORK_CONNECTED, 0, OS_FALSE);
            osal_set_network_state_int(OSAL_NS_NETWORK_USED, 0, OS_TRUE);

            /* The following four lines are silly stuff to reset
               the ESP32 wifi after soft reboot. I assume that this will be fixed and
               become unnecessary at some point.
             */
/*             WiFi.mode(WIFI_OFF);
            WiFi.mode(WIFI_STA);
            WiFi.disconnect();
            WiFi.getMode();
            WiFi.status(); */

            wifistate.network_connected = wifistate.wifi_was_connected = OS_FALSE;
            wifistate.wifi_init_failed_now = OS_FALSE;
            os_get_timer(&wifistate.wifi_step_timer);
            wifistate.wifi_boot_timer = wifistate.wifi_step_timer;

            /* Power management off. REALLY REALLY IMPORTANT, OTHERWISE WIFI WILL CRAWL.
             */
            esp_wifi_set_ps(WIFI_PS_NONE);

            wifistate.wifi_init_step = OSAL_WIFI_INIT_STEP2;
            break;

        case OSAL_WIFI_INIT_STEP2:
            if (os_has_elapsed(&wifistate.wifi_step_timer, 100))
            {
                /* Start the WiFi.
                 */
                if (!wifistate.wifi_multi_on)
                {
                    /* Initialize using static configuration.
                     */
                    if (wifistate.no_dhcp)
                    {
                        /* Some default network parameters.
                         */
                        //IPAddress ip_address(192, 168, 1, 195);
                        //osal_arduino_ip_from_str(ip_address, wifistate.ip_address);

                        /* Warning: ESP does not follow same argument order as arduino,
                           one below is for ESP32.
                         */
                        /* if (!WiFi.config(ip_address, wifistate.gateway_address,
                            wifistate.subnet_mask,
                            wifistate.dns_address, wifistate.dns_address_2))
                        {
                            osal_debug_error("Static IP configuration failed");
                        } */
                    }

                    osal_get_network_state_str(OSAL_NS_WIFI_NETWORK_NAME, 0,
                        wifi_net_name, sizeof(wifi_net_name));
                    osal_get_network_state_str(OSAL_NS_WIFI_PASSWORD, 0,
                        wifi_net_password, sizeof(wifi_net_password));
                    // WiFi.begin(wifi_net_name, wifi_net_password);
                }

                os_get_timer(&wifistate.wifi_step_timer);
                wifistate.wifi_init_step = OSAL_WIFI_INIT_STEP3;
                osal_trace("Connecting wifi");
            }
            break;

        case OSAL_WIFI_INIT_STEP3:
#if OSAL_SUPPORT_WIFI_MULTI
            if (!wifistate.wifi_multi_on)
            {
                wifistate.network_connected = (os_boolean) (WiFi.status() == WL_CONNECTED);
            }
            else
            {
                wifistate.network_connected = (os_boolean) (wifiMulti.run() == WL_CONNECTED);
            }
#else
            // wifistate.network_connected = (os_boolean) (WiFi.status() == WL_CONNECTED);
            wifistate.network_connected = OS_FALSE;
#endif

            /* If no change in connection status:
               - If we are connected or connection has never failed (boot), or
                 not connected, return appropriate status code. If not con
             */
            if (wifistate.network_connected == wifistate.wifi_was_connected)
            {
                if (wifistate.network_connected)
                {
                    s = OSAL_SUCCESS;
                    break;
                }

                if (wifistate.wifi_init_failed_now)
                {
                    s = OSAL_STATUS_FAILED;
                }

                else
                {
                    if (os_has_elapsed(&wifistate.wifi_step_timer, 10000))
                    {
                        wifistate.wifi_init_failed_now = OS_TRUE;
                        wifistate.wifi_init_failed_once = OS_TRUE;
                        osal_trace("Unable to connect Wifi");
                        osal_error(OSAL_ERROR, eosal_mod, OSAL_STATUS_NO_WIFI, OS_NULL);
                    }

                    s = wifistate.wifi_init_failed_once
                        ? OSAL_STATUS_FAILED : OSAL_PENDING;
                }

                break;
            }

            /* Save to detect connection state changes.
             */
            wifistate.wifi_was_connected = wifistate.network_connected;

            /* If this is connect
             */
            if (wifistate.network_connected)
            {
                s = OSAL_SUCCESS;
                //osal_trace_str("Wifi network connected: ", WiFi.SSID().c_str());

                /* SETUP TO RECEIVE multicasts from this IP address.
                 */
                /* IPAddress ip = WiFi.localIP();
                String addrstr = DisplayAddress(ip);
                const os_char *p = addrstr.c_str();
                os_strncpy(sg.nic[OSAL_WIFI_NIC_IX].ip_address, p, OSAL_IPADDR_SZ); 
                osal_error(OSAL_CLEAR_ERROR, eosal_mod, OSAL_STATUS_NO_WIFI, p); */
                osal_set_network_state_int(OSAL_NS_NETWORK_CONNECTED, 0, OS_TRUE);
#if OSAL_TRACE
                // osal_trace(addrstr.c_str());
#endif
            }

            /* Otherwise this is disconnect.
             */
            else
            {
//                wifistate.wifi_init_step = OSAL_WIFI_INIT_STEP1;   PEKKA TEST NOT TO REINITIALIZE
                osal_trace("Wifi network disconnected");
                s = OSAL_STATUS_FAILED;
            }

            break;
    }

    return s;
#endif    
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

#if 0
/* DPP Enrollee Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_dpp.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "qrcode.h"

#ifdef CONFIG_ESP_DPP_LISTEN_CHANNEL
#define EXAMPLE_DPP_LISTEN_CHANNEL_LIST     CONFIG_ESP_DPP_LISTEN_CHANNEL_LIST
#else
#define EXAMPLE_DPP_LISTEN_CHANNEL_LIST     "6"
#endif

#ifdef CONFIG_ESP_DPP_BOOTSTRAPPING_KEY
#define EXAMPLE_DPP_BOOTSTRAPPING_KEY   CONFIG_ESP_DPP_BOOTSTRAPPING_KEY
#else
#define EXAMPLE_DPP_BOOTSTRAPPING_KEY   0
#endif

#ifdef CONFIG_ESP_DPP_DEVICE_INFO
#define EXAMPLE_DPP_DEVICE_INFO      CONFIG_ESP_DPP_DEVICE_INFO
#else
#define EXAMPLE_DPP_DEVICE_INFO      0
#endif

static const char *TAG = "wifi dpp-enrollee";
wifi_config_t s_dpp_wifi_config;

static int s_retry_num = 0;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_dpp_event_group;

#define DPP_CONNECTED_BIT  BIT0
#define DPP_CONNECT_FAIL_BIT     BIT1
#define DPP_AUTH_FAIL_BIT           BIT2

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_ERROR_CHECK(esp_supp_dpp_start_listen());
        ESP_LOGI(TAG, "Started listening for DPP Authentication");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_dpp_event_group, DPP_CONNECT_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_dpp_event_group, DPP_CONNECTED_BIT);
    }
}

void dpp_enrollee_event_cb(esp_supp_dpp_event_t event, void *data)
{
    switch (event) {
    case ESP_SUPP_DPP_URI_READY:
        if (data != NULL) {
            esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();

            ESP_LOGI(TAG, "Scan below QR Code to configure the enrollee:\n");
            esp_qrcode_generate(&cfg, (const char *)data);
        }
        break;
    case ESP_SUPP_DPP_CFG_RECVD:
        memcpy(&s_dpp_wifi_config, data, sizeof(s_dpp_wifi_config));
        esp_wifi_set_config(ESP_IF_WIFI_STA, &s_dpp_wifi_config);
        ESP_LOGI(TAG, "DPP Authentication successful, connecting to AP : %s",
                 s_dpp_wifi_config.sta.ssid);
        s_retry_num = 0;
        esp_wifi_connect();
        break;
    case ESP_SUPP_DPP_FAIL:
        if (s_retry_num < 5) {
            ESP_LOGI(TAG, "DPP Auth failed (Reason: %s), retry...", esp_err_to_name((int)data));
            ESP_ERROR_CHECK(esp_supp_dpp_start_listen());
            s_retry_num++;
        } else {
            xEventGroupSetBits(s_dpp_event_group, DPP_AUTH_FAIL_BIT);
        }
        break;
    default:
        break;
    }
}

void dpp_enrollee_init(void)
{
    s_dpp_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_supp_dpp_init(dpp_enrollee_event_cb));
    /* Currently only supported method is QR Code */
    ESP_ERROR_CHECK(esp_supp_dpp_bootstrap_gen(EXAMPLE_DPP_LISTEN_CHANNEL_LIST, DPP_BOOTSTRAP_QR_CODE,
                    EXAMPLE_DPP_BOOTSTRAPPING_KEY, EXAMPLE_DPP_DEVICE_INFO));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_dpp_event_group,
                                           DPP_CONNECTED_BIT | DPP_CONNECT_FAIL_BIT | DPP_AUTH_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & DPP_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 s_dpp_wifi_config.sta.ssid, s_dpp_wifi_config.sta.password);
    } else if (bits & DPP_CONNECT_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 s_dpp_wifi_config.sta.ssid, s_dpp_wifi_config.sta.password);
    } else if (bits & DPP_AUTH_FAIL_BIT) {
        ESP_LOGI(TAG, "DPP Authentication failed after %d retries", s_retry_num);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    esp_supp_dpp_deinit();
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_dpp_event_group);
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    dpp_enrollee_init();
}
#endif


#endif
#endif
#endif
