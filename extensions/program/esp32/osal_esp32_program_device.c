/**

  @file    eosal/extensions/program/linux/osal_esp32_program_device.c
  @brief   Write IO device firmware to flash.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    1.7.2020

  The ESP32 comes with ready software update API, called OTA. The OTA update mechanism allows
  a device to update itself over IOCOM connection while the normal firmware is running.

  The default name for ESP32 program is firmware.bin.

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

    osalStatus status;
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


/**
****************************************************************************************************

  @brief Clear installation state, check if we need to validate the image, and setup NVS flash.
  @anchor osal_initialize_programming

  The osal_initialize_programming() function is called at boot to do a few tasks
  - It clears the installation state structure (for soft reboot).
  - Checks if are booting a new firmware image, which has not been verified yet
    (ESP_OTA_IMG_PENDING_VERIFY). If so calls diagnostics to verify the image (now always
    "all fine") and depending on verification success marks the image for use by cancelling
    rollback, or initiates rollback if new image is no good.

  @return  None.

****************************************************************************************************
*/
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


/**
****************************************************************************************************

  @brief Start the device programming.
  @anchor osal_start_device_programming

  The osal_start_device_programming() function is called when program transfer starts. The function
  sets up for program transfer.

  @return  OSAL_SUCCESS if the the installation can be started.

****************************************************************************************************
*/
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


/**
****************************************************************************************************

  @brief Append data to flash.
  @anchor osal_program_device

  The osal_program_device() function is called when data is received from iocom to write it
  to flash. Appending is done through 1024 byte buffer. At beginning of program the image
  header is verified.

  @return  OSAL_SUCCESS if all is fine. Other values indicate an error.

****************************************************************************************************
*/
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


/**
****************************************************************************************************

  @brief Install succesfully transferred the .bin firmvare.
  @anchor osal_finish_device_programming

  The osal_finish_device_programming() function is called when all data in firmvare program has
  been transferred. The function closes the validates the file and if all good, marks it
  for boot.

  @return  None.

****************************************************************************************************
*/
void osal_finish_device_programming(
    os_uint checksum)
{
    esp_err_t err;

    if (osal_istate.buf) {
        if (osal_flush_programming_buffer())
        {
            osal_istate.status = OSAL_STATUS_FAILED;
            goto getout;
        }

        err = esp_ota_end(osal_istate.update_handle);
        if (err != ESP_OK) {
            if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
                osal_istate.status = OSAL_STATUS_CHECKSUM_ERROR;
                osal_debug_error("image validation failed, image is corrupted");
            }
            else {
                osal_istate.status = OSAL_STATUS_FAILED;
                osal_debug_error_str("esp_ota_end failed: ", esp_err_to_name(err));
            }
            goto getout;
        }

        err = esp_ota_set_boot_partition(osal_istate.update_partition);
        if (err != ESP_OK) {
            osal_debug_error_str("esp_ota_set_boot_partition failed: ", esp_err_to_name(err));
            osal_istate.status = OSAL_STATUS_FAILED;
            goto getout;
        }
        osal_istate.status = OSAL_COMPLETED;

        osal_trace("prepare to restart system!");
        osal_cancel_device_programming();
        esp_restart();
        return;
    }

    osal_istate.status = OSAL_STATUS_FAILED;

getout:
    osal_cancel_device_programming();
}


/**
****************************************************************************************************

  @brief Check for errors in device programming.
  @anchor get_device_programming_status

  The get_device_programming_status() function can be called after osal_finish_device_programming()
  to polll if programming has failed. Notice that success value OSAL_COMPLETED may never be
  returned, if the IO device can reboots on successfull completion

  @return  OSAL_PENDING Installer still running.
           OSAL_COMPLETED Installation succesfully completed.
           Other values indicate an device programming error.

****************************************************************************************************
*/
osalStatus get_device_programming_status(void)
{
    return osal_istate.status;
}


/**
****************************************************************************************************

  @brief Cancel program installation.
  @anchor osal_cancel_device_programming

  The osal_cancel_device_programming() function can be called when for example part of firmware
  program has been transferred, but the socket connection breaks, etc.
  @return  None

****************************************************************************************************
*/
void osal_cancel_device_programming(void)
{
    if (osal_istate.buf) {
        osal_trace("programming buffer released");
        os_free(osal_istate.buf, OSAL_PROG_BLOCK_SZ);
        osal_istate.buf = OS_NULL;
    }
}


#if OSAL_ENABLE_ROLLBACK
/**
****************************************************************************************************

  @brief Verify header.
  @anchor osal_program_verify_hdr

  The osal_program_verify_hdr() function checks that header is valid.
  @return  OSAL_SUCCESS if all is fine. Other values indicate an error.

****************************************************************************************************
*/
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
/**
****************************************************************************************************

  @brief Check that new image works.
  @anchor osal_program_diagnostic

  The osal_program_diagnostic() function is called when a newly installed image boots for the
  first time. It should check that image is operational.
  @return  OS_TRUE if all is good, OS_FALSE otherwisee

****************************************************************************************************
*/
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


/**
****************************************************************************************************

  @brief Append received data to buffer.
  @anchor osal_buffer_append

  The osal_buffer_append() function appends up to *buf_sz bytes to buffer and advances *buf
  and *buf_sz by number of bytes appended. Idea here is to write program to flash in 1204 byte
  blocks, regardless on size of chunks it is received.
  @param   buf Pointer to pointer to data to append. Moved by number of bytes appended.
  @param   buf_sz Pointer to number of data bytes in buf. Number of appended bytes is substracted
           from this.
  @return  None

****************************************************************************************************
*/
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


/**
****************************************************************************************************

  @brief Append received data to buffer.
  @anchor osal_flush_programming_buffer

  The osal_flush_programming_buffer() function is called when there is 1024 bytes in append
  bufffer, or for last chunk at any number of bytes. It writes append buffer content to the flash.
  @return  None

****************************************************************************************************
*/
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
/**
****************************************************************************************************

  @brief Print SHA256 hash.
  @anchor osal_print_sha256

  The osal_print_sha256() function is just for debugging to print out SHA256 hash.
  @param   image_hash Hash value to print, 32 bytes.
  @param   label Text label.
  @return  None

****************************************************************************************************
*/
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
