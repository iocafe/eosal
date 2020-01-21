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

/* Each group of three needs 4 bytes in resulting string, plus one byte for terminating '\0' and
 * one for '!' in beginning (used to separate encrypted passwords from non encrypted).
 */
#define OSAL_HASH_STR_SZ (4*OSAL_HASH_3_GROUPS + 2)

/* Convert binary data to password string (do not call from app)
 */
void osal_password_bin2str(
    os_char *str,
    os_memsz str_sz,
    const void *data,
    os_memsz data_sz,
    os_boolean prefix_with_excl_mark);

/* Genrate a random password.
 */
void osal_make_random_password(
    os_char *password,
    os_memsz password_sz);
