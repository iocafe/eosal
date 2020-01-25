/**

  @file    seacret/common/osal_seacret.h
  @brief   Seacret (random number) to secure this network node.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    25.1.2020

  We need one "seacret" datum to base on security on. Once we have a seacret, everything else
  can be "public" and accessible to anyone, at least in encrypred form.

  The seacret is generated by the device and never leaves it as is: The seacret is 256 bit
  (32 byte) random number which can be accessed only by the device.
  It is used to create password (by cryptographic SHA-256 hash) and as encryption key to

  A seacret is generated as needed by osal_make_random_seacret() function and stored in
  persistent storage block OS_PBNR_SEACRET. At later boots the seacret is loaded from persistent
  storage.

  Application should use only following four seacret related functions:
  - osal_get_seacret()    Get seacret as string. Seacret is loaded or generated as needed.
                          String buffer size if 46 characters (OSAL_SEACRET_STR_SZ).
  - osal_get_password()   Get password as string. Password is sort of limited "seacret". It
                          can be transferred over secure connection from device to server.
  - osal_hash_password()  Make cryptographic hash of password. Hash is public information and
                          can be displayed to user, etc. It can be used to compare if
                          password matches but cannot be used as password to gain access.
  - osal_forget_seacret() This function can be called by IO device putton push, etc. It
                          restores the security to default state that IO device can
                          be reconfigured. After calling this function IO device is no
                          loner part of any secure IO network.

  To summarize:
  - The seacret is 256 bit random number.
  - Seacter can be accessed by secure communication process only, it is not given out.
  - IO node's password is SHA-256 hash of the seacret.
  - IO node's hash password is SHA-256 of the node's password. So SHA-256 hash is run twice.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#if OSAL_SEACRET_SUPPORT


/** 256 bit hash is 32 bytes. Same as SHA256_DIGEST_LENGTH when openssl is used.
 */
#define OSAL_HASH_SZ 32

/** This makes 11 three byte groups.
 */
#define OSAL_HASH_3_GROUPS ((OSAL_HASH_SZ + 2)/3)

/** Each group of three needs 4 bytes in resulting string, plus one byte for terminating '\0' and
    one for '!' in beginning (used to separate encrypted passwords from non encrypted).
 */
#define OSAL_HASH_STR_SZ (4*OSAL_HASH_3_GROUPS + 2)

/* Get seacret. This is used for encrypting private key of TLS server so it can be saved
   as normal data.
 */
void osal_get_seacret(
    os_char *buf,
    os_memsz buf_sz);

/* Get password. This is used as IO node password.
 */
void osal_get_password(
    os_char *buf,
    os_memsz buf_sz);

/* Hash password. Runs second SHA-256 hash on password.
 */
void osal_hash_password(
    os_char *buf,
    os_memsz buf_sz,
    const os_char *password);

/* Forget the seacret (and password).
 */
void osal_forget_seacret(void);

/* Convert binary data to seacret string
 */
void osal_seacret_bin2str(
    os_char *str,
    os_memsz str_sz,
    const void *data,
    os_memsz data_sz,
    os_boolean prefix_with_excl_mark);

/* Create random seacret and password into osal_global structure.
 */
void osal_make_random_seacret(void);

/* Initialize seacret and password (if not initialized already)
 */
void osal_initialize_seacret(void);


#endif
