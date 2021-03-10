/**

  @file    tls/common/osal_aes_crypt.h
  @brief   Simple AES encryption and decryption routines.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    20.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_AES_CRYPT_H_
#define OSAL_AES_CRYPT_H_
#include "eosalx.h"

/* Encryption key size is 32 bytes, 256 bits.
 */
#define OSAL_AES_KEY_SZ 32
#define OSAL_AES_BITS (OSAL_AES_KEY_SZ * 8)

typedef enum osalAesOperation {
    OSAL_AES_ENCRYPT,
    OSAL_AES_DECRYPT
}
osalAesOperation;

/* Encrypt or decrypt data, AES 256.
 */
void osal_aes_crypt(
    const os_uchar *data,
    os_memsz data_sz,
    os_uchar *buf,
    os_memsz buf_sz,
    const os_uchar key[OSAL_AES_KEY_SZ],
    osalAesOperation operation);

#endif
