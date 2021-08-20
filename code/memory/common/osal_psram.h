/**

  @file    memory/common/osal_psram.h
  @brief   Heap allocation from pseudo static RAM.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  In addition to RAM on chip, some microcontrollers can use pseudo static RAM trough SPI bus, etc.
  This PSRAM is typically large, but much slower than on chip RAM.
  Memory

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_PSRAM_H_
#define OSAL_PSRAM_H_
#include "eosal.h"

#if OSAL_PSRAM_SUPPORT

/* Allocate a block of memory from PSRAM.
 */
os_char *osal_psram_alloc(
    os_memsz request_bytes,
    os_memsz *allocated_bytes);

/* Release a block of memory from PSRAM.
 */
void osal_psram_free(
    void *memory_block,
    os_memsz bytes);

#else

/* If there is no PSRAM support, fall back to os_malloc() and os_free().
 */
#define osal_psram_alloc(n, a) os_malloc((n), (a))
#define osal_psram_free(b,n) os_free((b), (n))

#endif

#endif
