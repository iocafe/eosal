/**

  @file    tls/common/osal_crypto_hash.h
  @brief   Cryptographic hash.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Passwords are cryptographically hashed. Cryptographic hashes of two passwords can be used to
  check if passwords do match, but are not secret. One cannot get original password from
  it's cryptographic hash.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_CRYPTO_HASH_H_
#define OSAL_CRYPTO_HASH_H_
#include "eosalx.h"

/* Calculate SHA-256 cryptographic hash (as binary) of buffer given as argument
 */
void osal_sha256(
    const os_uchar *d,
    os_memsz n,
    os_uchar *md);

#endif
