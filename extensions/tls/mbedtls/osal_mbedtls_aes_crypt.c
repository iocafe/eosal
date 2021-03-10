/**

  @file    tls/common/osal_mbedtls_aes_crypt.c
  @brief   Simple AES encryption/decryption function using Mbed TLS.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    20.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_TLS_SUPPORT==OSAL_TLS_MBED_WRAPPER
#include "mbedtls/aes.h"

/**
****************************************************************************************************

  @brief Encrypt or decrypt data, AES 256.

  @param   data Data to encrypt or decrypt.
  @param   data_sz Data size in bytes. Any data size will do, doesn't have to be multple of 16.
  @param   buf Pointer to buffer where to place encrypted/decrypted data, always data_sz bytes
           are stored into buffer.
  @param   buf_sz Should be at least the same as data_sz. Used only to point out program errors.
  @param   key 256 bit encryption key, 32 bytes.
  @param   operation Either OSAL_AES_ENCRYPT or OSAL_AES_DECRYPT.

****************************************************************************************************
*/
void osal_aes_crypt(
    const os_uchar *data,
    os_memsz data_sz,
    os_uchar *buf,
    os_memsz buf_sz,
    const os_uchar key[OSAL_AES_KEY_SZ],
    osalAesOperation operation)
{
    mbedtls_aes_context aes;
    unsigned char iv[16];
    os_uchar *use_buf;
    const os_uchar *use_data;
    os_memsz use_sz;

    osal_debug_assert(buf_sz >= data_sz);

    os_memclear(iv, sizeof(iv));

    /* AES-CBC buffer encryption/decryption Length should be a multiple of the block size (16 bytes)
     */
    if (buf_sz & 0xF) {
        use_sz = (buf_sz + 0xF) & ~0xF;
        use_buf = (os_uchar*)os_malloc(2 * use_sz, OS_NULL);
        os_memclear(use_buf, 2 * use_sz);
        os_memcpy(use_buf + use_sz, data, data_sz);
        use_data = use_buf + use_sz;
    }
    else {
        use_sz = buf_sz;
        use_buf = buf;
        use_data = data;
    }

    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, OSAL_AES_BITS);
    mbedtls_aes_crypt_cbc(&aes, (operation == OSAL_AES_ENCRYPT)
        ? MBEDTLS_AES_ENCRYPT : MBEDTLS_AES_DECRYPT,
        use_sz, iv, use_data, use_buf);

    mbedtls_aes_free(&aes);

    if (buf_sz & 0xF) {
        os_memcpy(buf, use_buf, data_sz);
        os_free(use_buf, 2 * use_sz);
    }
}

#endif
