/**

  @file    persistent/arduino/osal_esp32_nvs_persistent.c
  @brief   Save persistent parameters on ESP32, uses NVS API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  The NVS API used here is the preferred choice for saving persistent parameters on ESP32.

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

#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#define OSAL_STORAGE_NAMESPACE "eosal"

static os_boolean os_persistent_lib_initialized = OS_FALSE;

/** The persistent handle structure stores some data about an open block.
 */
typedef struct
{
    osPersistentBlockNr block_nr;   /* Persistent block number, see osPersistentBlockNr enum.  */
    os_int flags;       /* Flags given to os_persistent_open() */
    os_memsz required_sz; /* Read, number of bytes stored in NVS */

    os_char *buf;       /* Pointer to data buffer, OS_NULL if none */
    os_memsz buf_sz;    /* Requested buffer size in bytes for reads. Allocated buffer size for
                           writes. */

    os_memsz pos;       /* Read or write position */
}
osPersistentNvsHandle;


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
    esp_err_t err;

    /* Do not initialized again by load or save call.
     */
    os_persistent_lib_initialized = OS_TRUE;

    err = nvs_flash_init();
    if (err != ESP_OK) {
        osal_debug_error_int("nvs_flash_init() failed once, err=", err);

        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            /* NVS partition was truncated and needs to be erased. Retry nvs_flash_init.
             */
            err = nvs_flash_erase();
            if (err != ESP_OK) {
                osal_debug_error("nvs_flash_erase() failed");
            }

            err = nvs_flash_init();
            if (err != ESP_OK) {
                osal_debug_error("nvs_flash_init() failed after erase");
            }
        }
    }
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
  @param   flags OSAL_PERSISTENT_READ, OSAL_PERSISTENT_WRITE, or OSAL_PERSISTENT_WRITE_AT_ONCE.
           Difference between OSAL_PERSISTENT_WRITE and OSAL_PERSISTENT_WRITE_AT_ONCE is that
           the os_persistent_write() can be called multiple times, but for
           OSAL_PERSISTENT_WRITE_AT_ONCE only once. The latter doesn't need extra buffer
           allocation, so it is faster.
           Additional flag OSAL_PERSISTENT_SECRET enables reading or writing the secret, and must
           be given to open confidential context.

  @return  Persistant storage block handle, or OS_NULL if the function failed.

****************************************************************************************************
*/
osPersistentHandle *os_persistent_open(
    osPersistentBlockNr block_nr,
    os_memsz *block_sz,
    os_int flags)
{
    osPersistentNvsHandle *h;
    esp_err_t err;
    os_char nbuf[OSAL_NBUF_SZ + 1];
    size_t required_size;

#if IDF_VERSION_MAJOR >= 4   /* esp-idf version 4 */
    nvs_handle_t my_handle;
#else                        /* esp-idf version 3 */
    nvs_handle my_handle;
#endif

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
    h->block_nr = block_nr;
    h->flags = flags;

    /* If we are reading, read whole block immediately to buffer.
     */
    if ((flags & OSAL_PERSISTENT_WRITE) == 0)
    {
        /* Open NVS storage.
         */
         err = nvs_open(OSAL_STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
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
            if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
                osal_debug_error_int("nvs_get_blob failed, code=", err);
            }
            nvs_close(my_handle);
            goto failed;
        }
        if (block_sz) {
            *block_sz = required_size;
        }
        h->required_sz = required_size;

        /* Close NVS storage.
         */
        nvs_close(my_handle);
    }

    return (osPersistentHandle*)h;

failed:
    osal_psram_free(h->buf, h->buf_sz);
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
  @return  If all is fine, the function returns OSAL_SUCCESS. Other values can be returned
           to indicate an error.

****************************************************************************************************
*/
osalStatus os_persistent_close(
    osPersistentHandle *handle,
    os_int flags)
{
    esp_err_t err;
    os_char nbuf[OSAL_NBUF_SZ + 1];
    osalStatus s = OSAL_STATUS_FAILED;

#if IDF_VERSION_MAJOR >= 4   /* esp-idf version 4 */
    nvs_handle_t my_handle;
#else                        /* esp-idf version 3 */
    nvs_handle my_handle;
#endif

    osPersistentNvsHandle *h;
    h = (osPersistentNvsHandle*)handle;

    if (h == OS_NULL) {
        return OSAL_STATUS_FAILED;
    }

    if ((h->flags & OSAL_PERSISTENT_WRITE) && (h->flags & OSAL_PERSISTENT_AT_ONCE) == 0)
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
        err = nvs_set_blob(my_handle, nbuf, h->buf ? h->buf : "", h->pos);
        if (err != ESP_OK) {
            osal_debug_error_int("nvs_set_blob failed, code=", err);
            nvs_close(my_handle);
            goto failed;
        }

        /* Commit data to flash and close then NVS storage.
         */
        err = nvs_commit(my_handle);
        if (err != ESP_OK) {
            osal_debug_error_int("nvs_commit failed on block ", h->block_nr);
            nvs_close(my_handle);
            goto failed;
        }
        nvs_close(my_handle);
    }
    s = OSAL_SUCCESS;

failed:
    osal_psram_free(h->buf, h->buf_sz);
    os_free(h, sizeof(osPersistentNvsHandle));
    return s;
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
    esp_err_t err;
    os_ushort n;
    os_char nbuf[OSAL_NBUF_SZ + 1];
    size_t required_size;

#if IDF_VERSION_MAJOR >= 4   /* esp-idf version 4 */
    nvs_handle_t my_handle;
#else                        /* esp-idf version 3 */
    nvs_handle my_handle;
#endif

    osPersistentNvsHandle *h;
    h = (osPersistentNvsHandle*)handle;

    if (h == OS_NULL || buf_sz <= 0) {
        return -1;
    }

    /* Get block data from NVS.
     */
    if (h->buf == OS_NULL)
    {
        if (h->pos) {
            osal_debug_error_int("reading past end of block ", h->block_nr);
            return -1;
        }

        /* Open NVS storage.
         */
         err = nvs_open(OSAL_STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
         if (err != ESP_OK) {
            osal_debug_error_str("nvs_open failed for read on ", OSAL_STORAGE_NAMESPACE);
            return -1;
         }

        nbuf[0] = 'v';
        osal_int_to_str(nbuf + 1, sizeof(nbuf) - 1, h->block_nr);

        /* If we are reading whole block at once, we do not need temporary buffer.
         */
if (buf_sz == h->required_sz && 0)
        {
            err = nvs_get_blob(my_handle, nbuf, buf, &required_size);
            if (err != ESP_OK) {
                osal_debug_error_int("nvs_get_blob failed on block ", h->block_nr);
                nvs_close(my_handle);
                return -1;
            }

            h->pos = buf_sz;
            return buf_sz;
        }

        /* Allocate temporary buffer and read contnt.
         */
        h->buf = osal_psram_alloc(h->required_sz, OS_NULL);
        if (h->buf == OS_NULL) {
            nvs_close(my_handle);
            return -1;
        }
        h->buf_sz = h->required_sz;
        err = nvs_get_blob(my_handle, nbuf, h->buf, &required_size);
        if (err != ESP_OK) {
            osal_debug_error_int("nvs_get_blob failed on block ", h->block_nr);
            nvs_close(my_handle);
            return -1;
        }

        nvs_close(my_handle);
    }

    if (h->pos < h->buf_sz)
    {
        n = h->buf_sz - h->pos;
        if ((os_ushort)buf_sz < n) {
            n = (os_ushort)buf_sz;
        }
        os_memcpy(buf, h->buf +  h->pos, n);
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
  @param   buf Buffer Pointer to data to write (append).
  @param   buf_sz block_sz Block size in bytes.

  @return  OSAL_SUCCESS indicates all fine, other return values indicate on error.

****************************************************************************************************
*/
osalStatus os_persistent_write(
    osPersistentHandle *handle,
    const os_char *buf,
    os_memsz buf_sz)
{
    esp_err_t err;
    os_memsz sz, newsz;
    os_char *newbuf;
    os_char nbuf[OSAL_NBUF_SZ + 1];

#if IDF_VERSION_MAJOR >= 4   /* esp-idf version 4 */
    nvs_handle_t my_handle;
#else                        /* esp-idf version 3 */
    nvs_handle my_handle;
#endif

    osPersistentNvsHandle *h;
    h = (osPersistentNvsHandle*)handle;

    if (h == OS_NULL) {
        return OSAL_STATUS_FAILED;
    }

    if (h->flags & OSAL_PERSISTENT_AT_ONCE)
    {
        /* Open NVS storage.
         */
        err = nvs_open(OSAL_STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
        if (err != ESP_OK) {
            osal_debug_error_str("nvs_open failed for write on ", OSAL_STORAGE_NAMESPACE);
            return OSAL_STATUS_FAILED;
        }

        /* Write the block.
         */
        nbuf[0] = 'v';
        osal_int_to_str(nbuf + 1, sizeof(nbuf) - 1, h->block_nr);
        err = nvs_set_blob(my_handle, nbuf, buf ? buf : "", buf_sz);
        if (err != ESP_OK) {
            osal_debug_error_int("nvs_set_blob failed, code=", err);
            nvs_close(my_handle);
            return OSAL_STATUS_FAILED;
        }

        /* Commit data to flash and close then NVS storage.
         */
        err = nvs_commit(my_handle);
        if (err != ESP_OK) {
            osal_debug_error_int("nvs_commit failed on block ", h->block_nr);
            nvs_close(my_handle);
            return OSAL_STATUS_FAILED;
        }
        nvs_close(my_handle);
        return OSAL_SUCCESS;
    }

    /* Allocate buffer, or reallocate it to make it bigger.
     */
    if (h->pos + buf_sz > h->buf_sz) {
        sz = h->buf_sz > 0 ? 3 * h->buf_sz / 2 : 256;
        if (h->pos + buf_sz > sz) {
            sz = h->pos + buf_sz;
        }
        newbuf = osal_psram_alloc(sz, &newsz);
        if (newbuf == OS_NULL) {
            return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
        }
        if (h->pos) {
            os_memcpy(newbuf, h->buf, h->pos);
        }
        if (h->buf) {
            osal_psram_free(h->buf, h->buf_sz);
        }
        h->buf = newbuf;
        h->buf_sz = newsz;
    }

    os_memcpy(h->buf + h->pos, buf, buf_sz);
    h->pos += buf_sz;

    return OSAL_SUCCESS;
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
    esp_err_t err;

    if (flags & OSAL_PERSISTENT_DELETE_ALL) {
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            osal_debug_error("nvs_flash_erase() failed");
           return OSAL_STATUS_FAILED;
        }
        return OSAL_SUCCESS;
    }
    else {
        return os_save_persistent(block_nr, OS_NULL, 0, OS_TRUE);
    }
}


#endif
