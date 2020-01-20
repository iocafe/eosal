/**

  @file    tls/common/osal_crypto_hash.c
  @brief   Cryptographic hash.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    20.1.2020

  Passwords are cryptographically hashed. Cryptographic hashes of two passwords can be used to
  check if passwords do match, but are not seacret. One cannot get original password from
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
    SHA256_Update(&c, d, n);
    SHA256_Final(md, &c);
    OPENSSL_cleanse(&c, sizeof(c));
}

#endif


/**
****************************************************************************************************

  @brief Convert 6 bit integer to ascii char
  @anchor osal_hash_asc

  The osal_hash_asc() function...

  @param   x Integer value 0-63 to convert to ASCII character. Highest two bits are ignored.
  @return  Ascii character corresponding to integer value x: '0'-'9', 'a'-'z', 'A'-'Z', '_' or '!'

****************************************************************************************************
*/
static os_char osal_hash_asc(
    os_uchar x)
{
    const os_uchar n_alpha = 'z' - 'a' + 1; /* 26 characters */
    x &= 0x3F;

    if (x < 10) return '0' + x;
    x -= 10;
    if (x < n_alpha) return 'a' + x;
    x -= n_alpha;
    if (x < n_alpha) return 'A' + x;
    x -= n_alpha;
    return x ? '!' : '_';
}


/**
****************************************************************************************************

  @brief Calculate SHA-256 cryptographic hash (as string) of password
  @anchor osal_hash_asc

  The osal_hash_password() function calculates SHA-256 cryptographic has of password given as
  argument and stores the result as string into buffer.

  @param   buf Buffer where to store the result.
  @param   password Password to encrypt.
  @return  None.

****************************************************************************************************
*/
void osal_hash_password(
    osal_hash buf,
    os_char *password)
{
    os_uchar *s, md[3*OSAL_HASH_3_GROUPS];
    os_char *p;
    os_short count;

    os_memclear(md, sizeof(md));
    osal_sha256((const os_uchar*)password, os_strlen(password), md);

    os_memclear(buf, OSAL_HASH_STR_SZ);
    p = buf;
    s = md;

    count = OSAL_HASH_3_GROUPS;
    while (count--)
    {
        *(p++) = osal_hash_asc(s[0]);
        *(p++) = osal_hash_asc(s[0] >> 6 | s[1] << 2);
        *(p++) = osal_hash_asc(s[1] >> 4 | s[2] << 4);
        *(p++) = osal_hash_asc(s[2] >> 2);
        s += 3;
    }
}

