/**

  @file    eosal_checksum.h
  @brief   Calculate checksum (modbus CRC).
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the iocom project and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_CHECKSUM_H_
#define OSAL_CHECKSUM_H_
#include "eosalx.h"

/* Enable/disable checksum test code.
 */
#define OSAL_CHECKSUM_TEST 0

/* Initialization value for the checksum calculation.
 */
#define OSAL_CHECKSUM_INIT 0xFFFF

/* Calculates checksum for buffer.
 */
os_ushort os_checksum(
    const os_char *buf,
    os_memsz n,
    os_ushort *append_to_checksum);

#if OSAL_CHECKSUM_TEST
/* Test the checksum code.
 */
int osal_test_checksum(
    void);
#endif

#endif
