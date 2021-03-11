/**

  @file    tls/common/osal_aes_crypt.c
  @brief   Simple AES encryption/decryption function.
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

  @brief Initialize global AES crypt key for device secret and private server password.

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

****************************************************************************************************
*/
void osal_initialize_aes_crypt_key(void)
{
    os_uchar *secret_crypt_key, u;
    os_int i;
#ifdef OSAL_AES_KEY
    const os_char aes_key[] = OSAL_AES_KEY, *p;
#endif

    /* Initialize with little more complex data than zeros.
     */
    secret_crypt_key = osal_global->secret_crypt_key;
    u = 177;
    for (i = 0; i<OSAL_AES_KEY_SZ; i++) {
        secret_crypt_key[i] = u;
        u += 17;
    }

    /* XOR with hard coded application key string.
     */
#ifdef OSAL_AES_KEY
    for (i = 0, p = aes_key; i<OSAL_AES_KEY_SZ && *p != '\0'; i++, p++) {
        secret_crypt_key[i] ^= (os_uchar)*(p++);
    }
#endif

    /* If we have CPUID and want to use it in encryption key, XOR it in.
     */
#if OSAL_AES_CRYPTO_WITH_CPUID
    osal_xor_cpuid(secret_crypt_key, OSAL_AES_KEY_SZ);
#endif
}

#endif
