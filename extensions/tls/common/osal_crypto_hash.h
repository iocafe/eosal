/**

  @file    tls/common/osal_crypto_hash.h
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

/* 256 bit hash is 32 bytes. Same as SHA256_DIGEST_LENGTH when openssl is used.
 */
#define OSAL_HASH_SZ 32

/* This makes 11 three byte groups.
 */
#define OSAL_HASH_3_GROUPS ((OSAL_HASH_SZ + 2)/3)

/* Each group of three needs 4 bytes in resulting string, plus one byte for terminating '\0'.
 */
#define OSAL_HASH_STR_SZ (4*OSAL_HASH_3_GROUPS + 1)

/* Crypto hash string type.
 */
typedef os_char osal_hash[OSAL_HASH_STR_SZ];

/* Calculate SHA-256 cryptographic hash (as binary) of buffer given as argument
 */
void osal_sha256(
    const os_uchar *d,
    os_memsz n,
    os_uchar *md);

/* Calculate SHA-256 cryptographic hash (as string) of password given as argument
 */
void osal_hash_password(
    osal_hash buf,
    os_char *password);
