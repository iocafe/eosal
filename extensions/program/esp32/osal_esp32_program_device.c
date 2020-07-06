/**

  @file    eosal/extensions/program/linux/osal_esp32_program_device.c
  @brief   Write IO device firmware to flash.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    1.7.2020

  The ESP32 comes with ready software update API, called OTA. The OTA update mechanism allows
  a device to update itself over IOCOM connection while the normal firmware is running.

  OTA requires configuring the Partition Table of the device with at least two “OTA app slot”
  partitions (ieota_0 and ota_1) and an “OTA Data Partition”. The OTA operation functions write
  a new app firmware image to whichever OTA app slot is not currently being used for booting.
  Once the image is verified, the OTA Data partition is updated to specify that this image
  should be used for the next boot.

  - https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/ota.html
  - https://github.com/scottchiefbaker/ESP-WebOTA/tree/master/src

  Note 3.7.2020: To enable rollback staging version of arduino-esp32.git needs to be installed.
  This version has bug that it needs "SimpleBLE" listen in platformio.ini lib_deps.

  platform_packages =
    framework-arduinoespressif32 @ https://github.com/espressif/arduino-esp32.git

  https://docs.platformio.org/en/latest/platforms/espressif32.html#using-arduino-framework-with-staging-version

  ESP-IDF: Copyright (C) 2015-2019 Espressif Systems. This source code is licensed under
  the Apache License 2.0.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_DEVICE_PROGRAMMING_SUPPORT

#include "esp_system.h"
#include "esp_event.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

/* Define enable rollback to enable software upgrade rollback. See notes in beginning of this file.
 */
#define OSAL_ENABLE_ROLLBACK 1
#define OSAL_ENABLE_DIAGNOSTICS 0

/* SHA-256 digest length */
#define OSAL_PROG_HASH_LEN 32

/* Number of bytes to write with one esp_ota_write() call
 */
#define OSAL_PROG_BLOCK_SZ 1024

/* Total program header size.
 */
#if OSAL_ENABLE_ROLLBACK
    #define OSAL_PROG_N_HDR_BYTES \
        (sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
#else
    #define OSAL_PROG_N_HDR_BYTES OSAL_PROG_BLOCK_SZ
#endif

/* Installer state structure stores what the installer is "doing now".
 */
typedef struct
{
    const esp_partition_t *configured;
    const esp_partition_t *running;
    const esp_partition_t *update_partition;
    esp_ota_handle_t update_handle;
    os_boolean hdr_verified;

    uint8_t *buf;
    os_memsz n;
}
osalInstallerState;

static osalInstallerState osal_istate;

/* Forward referred static functions.
 */
#if OSAL_ENABLE_ROLLBACK
static osalStatus osal_program_verify_hdr(
    void);
#endif

static void osal_buffer_append(
    os_char **buf,
    os_memsz *buf_sz);

static osalStatus osal_flush_programming_buffer(
    void);


#if OSAL_ENABLE_ROLLBACK
static os_boolean osal_program_diagnostic(void);
#endif

/* Do we want to trace print SHA hashes
 */
#define OSAL_PROG_TRACE_SHA (OSAL_TRACE > 0)

#if OSAL_PROG_TRACE_SHA
    static void osal_print_sha256 (
        const uint8_t *image_hash,
        const os_char *label);
#else
    #define osal_print_sha256 (h,l)
#endif


void osal_initialize_programming(void)
{
    uint8_t sha_256[OSAL_PROG_HASH_LEN] = { 0 };
    esp_partition_t partition;
    esp_err_t err;
#if OSAL_ENABLE_ROLLBACK
    const esp_partition_t *running;
    esp_ota_img_states_t ota_state;
    bool diagnostic_is_ok;
#endif

    os_memclear(&osal_istate, sizeof(osal_istate));

    if (OSAL_PROG_N_HDR_BYTES > OSAL_PROG_BLOCK_SZ)
    {
        osal_debug_error("Buffer size mismatch: OSAL_PROG_BLOCK_SZ < OSAL_PROG_BLOCK_SZ");
    }

    /* get sha256 digest for the partition table
     */
    partition.address   = ESP_PARTITION_TABLE_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type      = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    osal_print_sha256(sha_256, "SHA-256 for the partition table: ");

    /* get sha256 digest for bootloader
     */
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    osal_print_sha256(sha_256, "SHA-256 for bootloader: ");

    /* Get sha256 digest for running partition
     */
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    osal_print_sha256(sha_256, "SHA-256 for current firmware: ");

#if OSAL_ENABLE_ROLLBACK
    running = esp_ota_get_running_partition();
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
        {
            diagnostic_is_ok = osal_program_diagnostic();
            if (diagnostic_is_ok) {
                osal_trace("diagnostics completed successfully! continuing execution ...");
                esp_ota_mark_app_valid_cancel_rollback();
            }
            else {
                osal_trace("diagnostics failed! Start rollback to the previous version ...");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }
#endif

    /* Initialize NVS. */
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* OTA app partition table has a smaller NVS partition size than the non-OTA
           partition table. This size mismatch may cause NVS initialization to fail.
           If this happens, we erase NVS partition and initialize NVS again.
         */
        err = nvs_flash_erase();
        osal_debug_assert(err == ESP_OK);
        err = nvs_flash_init();
    }
    osal_debug_assert(err == ESP_OK);
}


osalStatus osal_start_device_programming(void)
{
    osal_cancel_device_programming();

    osal_trace("start programming");

    osal_istate.configured = esp_ota_get_boot_partition();
    osal_istate.running = esp_ota_get_running_partition();

    if (osal_istate.configured != osal_istate.running) {
        osal_trace_int("configured OTA boot partition at offset: ", osal_istate.configured->address);
        osal_trace_int("but running from offset (corrupted flas?): ", osal_istate.running->address);
    }

    osal_trace_int("running partition type: ", osal_istate.running->type);
    osal_trace_int("subtype: ", osal_istate.running->subtype);
    osal_trace_int("partition offset: ", osal_istate.running->address);

    osal_istate.update_partition = esp_ota_get_next_update_partition(NULL);
    osal_debug_assert(osal_istate.update_partition != NULL);
    osal_trace_int("writing to partition subtype: ", osal_istate.update_partition->subtype);
    osal_trace_int("at offset: ", osal_istate.update_partition->address);

    osal_istate.buf = (uint8_t*)os_malloc(OSAL_PROG_BLOCK_SZ, OS_NULL);
    osal_istate.n = 0;
    osal_istate.hdr_verified = OS_FALSE;
    osal_istate.update_handle = 0;

    return OSAL_SUCCESS;
}


osalStatus osal_program_device(
    os_char *buf,
    os_memsz buf_sz)
{
    esp_err_t err;

    if (osal_istate.buf == OS_NULL) {
        return OSAL_STATUS_FAILED;
    }

    while (buf_sz > 0)
    {
        /* Append to buffer
         */
        osal_buffer_append(&buf, &buf_sz);

        /* If the verification has not been done.
         */
        if (!osal_istate.hdr_verified) {
            if (osal_istate.n < OSAL_PROG_N_HDR_BYTES) {
                return OSAL_SUCCESS;
            }

#if OSAL_ENABLE_ROLLBACK
            if (osal_program_verify_hdr()) {
                goto getout;
            }
#endif

            err = esp_ota_begin(osal_istate.update_partition, OTA_SIZE_UNKNOWN, &osal_istate.update_handle);
            if (err != ESP_OK) {
                osal_debug_error_str("esp_ota_begin failed: ", esp_err_to_name(err));
                goto getout;
            }
            osal_trace("esp_ota_begin succeeded");

            osal_istate.hdr_verified = OS_TRUE;
        }

        if (osal_istate.n == OSAL_PROG_BLOCK_SZ) {
            if (osal_flush_programming_buffer()) {
                goto getout;
            }
        }
        else {
            break;
        }
    }

    /* Success if we got everything written or buffered.
     */
    if (buf_sz == 0) {
        return OSAL_SUCCESS;
    }

getout:
    osal_cancel_device_programming();
    return OSAL_STATUS_FAILED;
}

osalStatus osal_finish_device_programming(
    os_uint checksum)
{
    esp_err_t err;

    if (osal_istate.buf) {
        if (osal_flush_programming_buffer())
        {
            osal_cancel_device_programming();
            return OSAL_STATUS_FAILED;
        }

        err = esp_ota_end(osal_istate.update_handle);
        if (err != ESP_OK) {
            if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
                osal_debug_error("image validation failed, image is corrupted");
            }
            osal_debug_error_str("esp_ota_end failed: ", esp_err_to_name(err));
            goto getout;
        }

        err = esp_ota_set_boot_partition(osal_istate.update_partition);
        if (err != ESP_OK) {
            osal_debug_error_str("esp_ota_set_boot_partition failed: ", esp_err_to_name(err));
            goto getout;
        }
        osal_trace("prepare to restart system!");

        osal_cancel_device_programming();
        esp_restart();
        return OSAL_SUCCESS;
    }

getout:
    osal_cancel_device_programming();
    return OSAL_STATUS_FAILED;
}

void osal_cancel_device_programming(void)
{
    if (osal_istate.buf) {
        osal_trace("programming buffer released");
        os_free(osal_istate.buf, OSAL_PROG_BLOCK_SZ);
        osal_istate.buf = OS_NULL;
    }
}


#if OSAL_ENABLE_ROLLBACK
static osalStatus osal_program_verify_hdr(
    void)
{
    esp_app_desc_t new_app_info;
    esp_app_desc_t running_app_info;
    esp_app_desc_t invalid_app_info;
    const esp_partition_t* last_invalid_app;

    // check current version with downloading
    os_memcpy(&new_app_info, &osal_istate.buf[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
    osal_trace_str("new firmware version:", new_app_info.version);

    if (esp_ota_get_partition_description(osal_istate.running, &running_app_info) == ESP_OK) {
        osal_trace_str("running firmware version: ", running_app_info.version);
    }

    last_invalid_app = esp_ota_get_last_invalid_partition();
    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
        osal_trace_str("last invalid firmware version: ", invalid_app_info.version);
    }

    /* Check current version with last invalid partition
     */
    if (last_invalid_app != NULL) {
        if (os_memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
            osal_debug_error_str("New version is the same as invalid version.\n"
                "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);

            return OSAL_STATUS_FAILED;
        }
    }
    return OSAL_SUCCESS;
}
#endif

#if OSAL_ENABLE_ROLLBACK
static os_boolean osal_program_diagnostic(void)
{
#if OSAL_ENABLE_DIAGNOSTICS
    gpio_config_t io_conf;
    os_boolean diagnostic_is_ok;

    io_conf.intr_type    = GPIO_PIN_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    osal_trace("Diagnostics (5 sec)...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    diagnostic_is_ok = gpio_get_level(CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);

    gpio_reset_pin(CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);
    return diagnostic_is_ok;
#else
    return OS_TRUE;
#endif
}
#endif

static void osal_buffer_append(
    os_char **buf,
    os_memsz *buf_sz)
{
    os_memsz n;

    n = *buf_sz;
    if (n > OSAL_PROG_BLOCK_SZ - osal_istate.n) {
        n = OSAL_PROG_BLOCK_SZ - osal_istate.n;
    }
    os_memcpy(osal_istate.buf + osal_istate.n, *buf, n);
    *buf += n;
    *buf_sz -= n;
    osal_istate.n += n;
}

static osalStatus osal_flush_programming_buffer(void)
{
    esp_err_t err;

    if (osal_istate.n)
    {
        err = esp_ota_write(osal_istate.update_handle, osal_istate.buf, osal_istate.n);
        osal_istate.n = 0;
        return (err == ESP_OK) ? OSAL_SUCCESS : OSAL_STATUS_FAILED;
    }

    return OSAL_SUCCESS;
}


#if OSAL_PROG_TRACE_SHA
static void osal_print_sha256 (
    const uint8_t *image_hash,
    const os_char *label)
{
    os_char hash_print[OSAL_PROG_HASH_LEN * 2 + 1];
    os_int i;
    uint8_t c;

    hash_print[OSAL_PROG_HASH_LEN * 2] = '\0';
    for (i = 0; i < OSAL_PROG_HASH_LEN; ++i) {
        c = (image_hash[i] >> 4);
        c = c > 9 ? c + 'A' : c + '0';
        hash_print[i * 2] = c;
        c = (image_hash[i] & 0xF);
        c = c > 9 ? c + 'A' : c + '0';
        hash_print[i * 2+1] = c;
    }
    osal_trace_str(label, hash_print);
}
#endif

#endif




#if 0
/* OTA example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "protocol_examples_common.h"
#include "errno.h"

#if CONFIG_EXAMPLE_CONNECT_WIFI
#include "esp_wifi.h"
#endif

#define BUFFSIZE 1024
#define HASH_LEN 32 /* SHA-256 digest length */

static const char *TAG = "native_ota_example";
/*an ota data write buffer ready to write to the flash*/
static char ota_write_data[BUFFSIZE + 1] = { 0 };
extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");

extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

xxx

static void print_sha256 (const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s: %s", label, hash_print);
}

static void ota_example_task(void *pvParameter)
{
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI(TAG, "Starting OTA example");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    esp_http_client_config_t config = {
        .url = CONFIG_EXAMPLE_FIRMWARE_UPG_URL,
        .cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = CONFIG_EXAMPLE_OTA_RECV_TIMEOUT,
    };

#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN
    char url_buf[OTA_URL_SIZE];
    if (strcmp(config.url, "FROM_STDIN") == 0) {
        example_configure_stdin_stdout();
        fgets(url_buf, OTA_URL_SIZE, stdin);
        int len = strlen(url_buf);
        url_buf[len - 1] = '\0';
        config.url = url_buf;
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong firmware upgrade image url");
        abort();
    }
#endif

#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialise HTTP connection");
        task_fatal_error();
    }
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        task_fatal_error();
    }
    esp_http_client_fetch_headers(client);

    update_partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);

    int binary_file_length = 0;
    /*deal with all receive packet*/
    bool image_header_was_checked = false;
    while (1) {
        int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
        if (data_read < 0) {
            ESP_LOGE(TAG, "Error: SSL data read error");
            http_cleanup(client);
            task_fatal_error();
        } else if (data_read > 0) {
            if (image_header_was_checked == false) {
                esp_app_desc_t new_app_info;
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
                    // check current version with downloading
                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

                    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
                    }

                    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_app_info;
                    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
                    }

                    // check current version with last invalid partition
                    if (last_invalid_app != NULL) {
                        if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
                            ESP_LOGW(TAG, "New version is the same as invalid version.");
                            ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                            ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
                            http_cleanup(client);
                            infinite_loop();
                        }
                    }
#ifndef CONFIG_EXAMPLE_SKIP_VERSION_CHECK
                    if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                        ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
                        http_cleanup(client);
                        infinite_loop();
                    }
#endif

                    image_header_was_checked = true;

                    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        http_cleanup(client);
                        task_fatal_error();
                    }
                    ESP_LOGI(TAG, "esp_ota_begin succeeded");
                } else {
                    ESP_LOGE(TAG, "received package is not fit len");
                    http_cleanup(client);
                    task_fatal_error();
                }
            }
            err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
                http_cleanup(client);
                task_fatal_error();
            }
            binary_file_length += data_read;
            ESP_LOGD(TAG, "Written image length %d", binary_file_length);
        } else if (data_read == 0) {
           /*
            * As esp_http_client_read never returns negative error code, we rely on
            * `errno` to check for underlying transport connectivity closure if any
            */
            if (errno == ECONNRESET || errno == ENOTCONN) {
                ESP_LOGE(TAG, "Connection closed, errno = %d", errno);
                break;
            }
            if (esp_http_client_is_complete_data_received(client) == true) {
                ESP_LOGI(TAG, "Connection closed");
                break;
            }
        }
    }
    ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);
    if (esp_http_client_is_complete_data_received(client) != true) {
        ESP_LOGE(TAG, "Error in receiving complete file");
        http_cleanup(client);
        task_fatal_error();
    }

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        }
        ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        http_cleanup(client);
        task_fatal_error();
    }
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
    return ;
}

static bool diagnostic(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type    = GPIO_PIN_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Diagnostics (5 sec)...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    bool diagnostic_is_ok = gpio_get_level(CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);

    gpio_reset_pin(CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);
    return diagnostic_is_ok;
}

void app_main(void)
{
    uint8_t sha_256[HASH_LEN] = { 0 };
    esp_partition_t partition;

    // get sha256 digest for the partition table
    partition.address   = ESP_PARTITION_TABLE_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_MAX_LEN;
    partition.type      = ESP_PARTITION_TYPE_DATA;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for the partition table: ");

    // get sha256 digest for bootloader
    partition.address   = ESP_BOOTLOADER_OFFSET;
    partition.size      = ESP_PARTITION_TABLE_OFFSET;
    partition.type      = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);
    print_sha256(sha_256, "SHA-256 for bootloader: ");

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
    print_sha256(sha_256, "SHA-256 for current firmware: ");

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // run diagnostic function ...
            bool diagnostic_is_ok = diagnostic();
            if (diagnostic_is_ok) {
                ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution ...");
                esp_ota_mark_app_valid_cancel_rollback();
            } else {
                ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
    }

    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // OTA app partition table has a smaller NVS partition size than the non-OTA
        // partition table. This size mismatch may cause NVS initialization to fail.
        // If this happens, we erase NVS partition and initialize NVS again.
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

#if CONFIG_EXAMPLE_CONNECT_WIFI
    /* Ensure to disable any WiFi power save mode, this allows best throughput
     * and hence timings for overall OTA operation.
     */
    esp_wifi_set_ps(WIFI_PS_NONE);
#endif // CONFIG_EXAMPLE_CONNECT_WIFI

    xTaskCreate(&ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);
}

#endif
