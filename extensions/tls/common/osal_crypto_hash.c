/**

  @file    tls/common/osal_crypto_hash.c
  @brief   Cryptographic hash.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    20.1.2020

  Passwords are cryptographically hashed. Cryptographic hashes of two passwords can be used to
  check if passwords do match, but are not secret. One cannot get original password from
  it's cryptographic hash.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_OPENSSL_SUPPORT

#include <openssl/sha.h>
#include <openssl/crypto.h>


/**
****************************************************************************************************

  @brief Calculate SHA-256 cryptographic hash (as binary) of buffer given as argument
  @anchor osal_sha256

  The osal_sha256() function...

  @param   d Source data for calculating the hash.
  @param   n Source data size in bytes.
  @param   md buffer for storing the hash result, 32 bytes (OSAL_HASH_SZ)
  @return  None

****************************************************************************************************
*/
void osal_sha256(
    const os_uchar *d,
    os_memsz n,
    os_uchar *md)
{
    SHA256_CTX c;

    SHA256_Init(&c);
    SHA256_Update(&c, d, (size_t)n);
    SHA256_Final(md, &c);
    OPENSSL_cleanse(&c, sizeof(c));
}

#endif


