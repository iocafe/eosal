/**

  @file    persistent/arduino/osal_esp32_nvs_persistent.c
  @brief   Save persistent parameters on ESP32, uses NVS API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/spi_flash.html
  Especially chapter "Concurrency Constraints for flash on SPI1"

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if (OSAL_PERSISTENT_SUPPORT == OSAL_PERSISTENT_NVS_STORAGE) || (OSAL_PERSISTENT_SUPPORT == OSAL_PERSISTENT_DEFAULT_STORAGE)

// #include <Arduino.h>
// #include <EEPROM.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
// #include "driver/gpio.h"


#define OSAL_STORAGE_NAMESPACE "eosal"

/** The persistent handle structure defined here is only for pointer type checking,
    it is never used.
 */
typedef struct
{
    osPersistentBlockNr block_nr;
    os_int flags;

    os_char *buf;
    os_memsz buf_sz;

    os_memsz pos;
}
osPersistentNvsHandle;


/* Forward referred static functions.
 */



/**
****************************************************************************************************

  @brief Initialize persistent storage for use.
  @anchor os_persistent_initialze

  The os_persistent_initialze() function.

  @param   prm Pointer to parameters for persistent storage. For this implementation path
           member sets path to folder where to keep parameter files. Can be OS_NULL if not
           needed.
  @return  None.

****************************************************************************************************
*/
void os_persistent_initialze(
    osPersistentParams *prm)
{
    os_ushort checksum;
    os_char buf[OSAL_NBUF_SZ];
    esp_err_t err;
    
    /* Do not initialized again by load or save call.
     */
    os_persistent_lib_initialized = OS_TRUE;

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
}


/**
****************************************************************************************************

  @brief Release any resources allocated for the persistent storage.
  @anchor os_persistent_shutdown

  The os_persistent_shutdown() function is just place holder for future implementations.
  @return  None.

****************************************************************************************************
*/
void os_persistent_shutdown(
    void)
{
}


/**
****************************************************************************************************

  @brief Get pointer to persistant data block within persistent storage.
  @anchor os_persistent_get_ptr

  If persistant storage is in micro-controller's flash, we can just get pointer to data block
  and data size. If persistant storage is on such media (file system, etc) that direct
  pointer is not available, the function returns OSAL_STATUS_NOT_SUPPORTED and function
  os_persistent_load() must be called to read the data to local RAM.

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   block Pointer to set to point then block (structure) in memory.
  @param   block_sz Set to block size in bytes.
  @param   flags OSAL_PERSISTENT_SECRET flag enables accessing the secret. It must be only
           given in safe context.
  @return  OSAL_SUCCESS of successful. Value OSAL_STATUS_NOT_SUPPORTED indicates that
           pointer cannot be aquired on this platform and os_persistent_load() must be
           called instead. Other values indicate an error.

****************************************************************************************************
*/
osalStatus os_persistent_get_ptr(
    osPersistentBlockNr block_nr,
    const os_uchar **block,
    os_memsz *block_sz,
    os_int flags)
{
#if OSAL_RELAX_SECURITY == 0
    /* Reading or writing seacred block requires secret flag. When this function
       is called for data transfer, there is no secure flag and thus secret block
       cannot be accessed to break security.
     */
    if ((block_nr == OS_PBNR_SECRET || block_nr == OS_PBNR_SERVER_KEY || block_nr == OS_PBNR_ROOT_KEY) &&
        (flags & OSAL_PERSISTENT_SECRET) == 0)
    {
        return OSAL_STATUS_NOT_AUTOHORIZED;
    }
#endif

    return OSAL_STATUS_NOT_SUPPORTED;
}


/**
****************************************************************************************************

  @brief Open persistent block for reading or writing.
  @anchor os_persistent_open

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   block_sz Pointer to integer where to store block size when reading the persistent block.
           This is intended to know memory size to allocate before reading.
  @param   flags OSAL_PERSISTENT_READ or OSAL_PERSISTENT_WRITE. Flag OSAL_PERSISTENT_SECRET
           enables reading or writing the secret, and must be only given in safe context.

  @return  Persistant storage block handle, or OS_NULL if the function failed.

****************************************************************************************************
*/
osPersistentHandle *os_persistent_open(
    osPersistentBlockNr block_nr,
    os_memsz *block_sz,
    os_int flags)
{
    osPersistentNvsHandle *h;
    // os_ushort first_free, pos, cscalc, n, nnow, p, sz;
    esp_err_t err;
    os_char nbuf[OSAL_NBUF_SIZE+1];
    // int i;

    /* Return zero block size if the function fails.
     */
    if (block_sz) {
        *block_sz = 0;
    }

    /* This implementation doesn not support nor simulate flash programming.
     */
    if (block_nr == OS_PBNR_FLASH_PROGRAM) {
        return OS_NULL;
    }

#if OSAL_RELAX_SECURITY == 0
    /* Reading or writing seacred block requires secret flag. When this function
       is called for data transfer, there is no secure flag and thus secret block
       cannot be accessed to break security.
     */
    if ((block_nr == OS_PBNR_SECRET || block_nr == OS_PBNR_SERVER_KEY || block_nr == OS_PBNR_ROOT_KEY) &&
        (flags & OSAL_PERSISTENT_SECRET) == 0)
    {
        return OS_NULL;
    }
#endif

    /* Make sure that NVS API is initialized.
     */
    if (!os_persistent_lib_initialized) {
        os_persistent_initialze(OS_NULL);
    }

    /* Allocate and clear handle, save block number and flags.
     */
    h = (osPersistentNvsHandle*)os_malloc(sizeof(osPersistentNvsHandle), OS_NULL);
    if (h == OS_NULL) {
        return OS_NULL;
    }
    os_memclear(h, sizeof(osPersistentNvsHandle));
    block->block_nr = block_nr;
    block->flags = flags;

    /* If we are reading, read whole block immediately to buffer.
     */
    if ((flags & OSAL_PERSISTENT_WRITE) == 0)
    {
        /* Open NVS storage.
         */
         err = nvs_open(OSAL_STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
         if (err != ESP_OK) {
            osal_debug_error_str("nvs_open failed for read on ", OSAL_STORAGE_NAMESPACE);
            goto failed;
         }

        /* Get block size within NVS.
         */
        nbuf[0] = 'v';
        osal_int_to_str(nbuf + 1, sizeof(nbuf) - 1, block_nr);
        err = nvs_get_blob(my_handle, nbuf, NULL, &required_size);
        if (err != ESP_OK || required_size <= 0) {
            if (err != ESP_ERR_NVS_NOT_FOUND) {
                osal_debug_error_int("nvs_get_blob failed on block ", block_nr);
            }
            goto failed;
        }
        if (block_sz) {
            *block_sz = required_size;
        }

        /* Allocate temporary buffer and read contnt.
         */
        h->buf = os_malloc(required_size, OS_NULL);
        if (h->buf == OS_NULL) {
            goto failed;
        }
        h->buf_sz = required_size;
        err = nvs_get_blob(my_handle, nbuf, h->buff, &required_size);
        if (err != ESP_OK) {
            osal_debug_error_int("nvs_get_blob failed on block ", block_nr);
            goto failed;
        }

        // Close
        nvs_close(my_handle);

        /* Verify checksum
         */
        /* n = block->sz;
        p = block->pos;
        cscalc = OSAL_CHECKSUM_INIT;
        while (n > 0)
        {
            nnow = n;
            if (sizeof(tmp) < nnow) nnow = sizeof(tmp);
            os_persistent_read_internal(tmp, p, nnow);
            os_checksum(tmp, nnow, &cscalc);
            p += nnow;
            n -= nnow;
        }
        if (cscalc != block->checksum) goto failed;
        */
    }

    return (osPersistentHandle*)h;

failed:
    os_free(h->buf, h->buf_sz);
    os_free(h, sizeof(osPersistentNvsHandle));
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Close persistent storage block.
  @anchor os_persistent_close

  @param   handle Persistant storage handle. It is ok to call the function with NULL handle,
           just nothing happens.
  @param   flags OSAL_STREAM_DEFAULT (0) is all was written to persistant storage.
           OSAL_STREAM_INTERRUPT flag is set if transfer was interrupted.
  @return  None.

****************************************************************************************************
*/
void os_persistent_close(
    osPersistentHandle *handle,
    os_int flags)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    osPersistentNvsHandle *h;
    h = (osPersistentNvsHandle*)handle;

    if (h == OS_NULL) return;

    if (h->flags & OSAL_PERSISTENT_WRITE)
    {
        /* Open NVS storage.
         */
        err = nvs_open(OSAL_STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
        if (err != ESP_OK) {
            osal_debug_error_str("nvs_open failed for write on ", OSAL_STORAGE_NAMESPACE);
            goto failed;
        }

        /* Write the block.
         */
        nbuf[0] = 'v';
        osal_int_to_str(nbuf + 1, sizeof(nbuf) - 1, h->block_nr);
        err = nvs_set_blob(my_handle, xx "run_time", h->buf, h->pos);
        if (err != ESP_OK) {
            osal_debug_error_int("nvs_set_blob failed on block ", h->block_nr);
        }

        /* Commit data to flash and close then NVS storage.
         */
        err = nvs_commit(my_handle);
        if (err != ESP_OK) {
            osal_debug_error_int("nvs_commit failed on block ", h->block_nr);
        }
        nvs_close(my_handle);
    }

failed:
    os_free(h->buf, h->buf_sz);
    os_free(h, sizeof(osPersistentNvsHandle));
}


/**
****************************************************************************************************

  @brief Read data from persistent parameter block.
  @anchor os_persistent_read

  The os_persistent_read() function reads data from persistant storage block. Load all
  parameters when micro controller starts, not during normal operation.

  @param   handle Persistant storage handle.
  @param   block Pointer to buffer where to load persistent data.
  @param   buf_sz Number of bytes to read to buffer.
  @return  Number of bytes read. Can be less than buf_sz if end of persistent block data has
           been reached. 0 is fine if at end. -1 Indicates an error.

****************************************************************************************************
*/
os_memsz os_persistent_read(
    osPersistentHandle *handle,
    os_char *buf,
    os_memsz buf_sz)
{
    os_ushort n;
    osPersistentNvsHandle *h;
    h = (osPersistentNvsHandle*)handle;

    if (h) if (hpos < h->buf_sz)
    {
        n = h->buf_sz - h->pos;
        if ((os_ushort)buf_sz < n) n = (os_ushort)buf_sz;
        os_memcpy(buf, h->buf +  h->read_pos, n);
        h->pos += n;
        return n;
    }

    return -1;
}


/**
****************************************************************************************************

  @brief Append data to persistent block.
  @anchor os_persistent_write

  os_persistent_write() function appends buffer to data to write.

  @param   handle Persistant storage handle.
  @param   buf Buffer into which data is written from.
  @param   buf_sz block_sz Block size in bytes.

  @return  OSAL_SUCCESS indicates all fine, other return values indicate on error.

****************************************************************************************************
*/
osalStatus os_persistent_write(
    osPersistentHandle *handle,
    const os_char *buf,
    os_memsz buf_sz)
{
    osPersistentNvsHandle *h;
    h = (osPersistentNvsHandle*)handle;

    if (h)
    {
        osal_control_interrupts(OS_FALSE);
        os_persistent_write_internal(buf, block->pos + block->sz, buf_sz);
        os_checksum(buf, buf_sz, &block->checksum);
        block->sz += (os_ushort)buf_sz;
        hdr.touched = 1;
        osal_control_interrupts(OS_TRUE);
        return OSAL_SUCCESS;
    }

    return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Wipe persistent data.

  The os_persistent_delete function deletes a persistent block or wipes out all persistent data.

  @param   block_nr Parameter block number.
  @param   flags Set OSAL_PERSISTENT_DELETE_ALL to delete all blocks or OSAL_PERSISTENT_DEFAULT
           to delete block number given as argument.
  @return  OSAL_SUCCESS if all good, other values indicate an error.

****************************************************************************************************
*/
osalStatus os_persistent_delete(
    osPersistentBlockNr block_nr,
    os_int flags)
{
    if (flags & OSAL_PERSISTENT_DELETE_ALL) {
        os_memclear((os_char*)&hdr, sizeof(hdr));
        osal_control_interrupts(OS_FALSE);
        os_persistent_write_internal((os_char*)&hdr, 0, sizeof(hdr));
        EEPROM.commit();
        osal_control_interrupts(OS_TRUE);
        return OSAL_SUCCESS;
    }
    else {
        return os_save_persistent(block_nr, OS_NULL, 0, OS_TRUE);
    }
}



#endif
