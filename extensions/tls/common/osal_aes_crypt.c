/**

  @file    tls/common/osal_aes_crypt.c
  @brief   Generate AES encryption/decryption key.
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
#if OSAL_AES_CRYPTO_SUPPORT

/**
****************************************************************************************************

  @brief Initialize AES crypt key for device secret and private server password.

  This function sets secret_crypt_key in osal_global structure. This is combination of simple
  fixed key, application hard coded key (from define) and optionally CPUID, which intends to
  unuquaely identifiy the individual computer.

  Application hard coded key is set by define OSAL_AES_KEY at build time.
  OSAL_AES_KEY="myseacretkey" can be defined in Cmakelists.txt, platformio.ini,
  in /coderoot/eosal/eosal_linux_config.h, etc.

  eosal_linux_config.h to set hard coded key and to force using CPUID even on PC:
    #define OSAL_AES_KEY "KebabMakkaraKioski"
    #define OSAL_AES_CRYPTO_WITH_CPUID 1

  The unique hardware identification is not normally used for PC computers, since
  we want to be able to make a working backup copy of a server computer. For microcontrollers
  we use this, if CPUID functionality support is available (OSAL_CPUID_SUPPORT define).

  This is not bullet proof. Serious microcontroller security should be done so that debugging
  ports, like JTAG and UART are permanently disabled at production version. On Windows and
  linux we should primarily depend on operating system security. But since we live in real
  world with real people, and errors happen with these, we want at least to make it really
  hard to get the device secret, user login or server's private key.

  @param   secret_crypt_key Pointer to buffer where to store the generated crypt key.
  @param   use_cpuid Nonzero if CPUID should be used (if cpyid is not supported for platform, it
           is not used regardless value of this argument. Set zero to disable using CPUID
           to enable making e working backup.

****************************************************************************************************
*/
void osal_initialize_aes_crypt_key(
    os_uchar secret_crypt_key[OSAL_AES_KEY_SZ],
    os_int use_cpuid)
{
    os_uchar buf[OSAL_AES_KEY_SZ], u;
    os_int i;
#ifdef OSAL_AES_KEY
    const os_char aes_key[] = OSAL_AES_KEY, *p;
#endif

    /* This function relies that these match, both should be 32 bytes
     */
    osal_debug_assert(OSAL_HASH_SZ == OSAL_AES_KEY_SZ);

    /* Initialize with little more complex data than zeros.
     */
    u = 177;
    for (i = 0; i<OSAL_AES_KEY_SZ; i++) {
        buf[i] = u;
        u += 17;
    }

    /* XOR with hard coded application key string.
     */
#ifdef OSAL_AES_KEY
    for (i = 0, p = aes_key; i<OSAL_AES_KEY_SZ && *p != '\0'; i++, p++) {
        buf[i] ^= (os_uchar)*(p++);
    }
#endif

    /* If we have CPUID and want to use it in encryption key, XOR it in.
     */
#if OSAL_CPUID_SUPPORT
    if (use_cpuid) {
        osal_sha256(buf, OSAL_AES_KEY_SZ, secret_crypt_key);
        os_memcpy(buf, secret_crypt_key, OSAL_AES_KEY_SZ);
        osal_xor_cpuid(buf, OSAL_AES_KEY_SZ);
    }
#endif

    /* Make and set as SHA 256 hash.
     */
    osal_sha256(buf, OSAL_AES_KEY_SZ, secret_crypt_key);
}

#endif
