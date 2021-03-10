/**

  @file    cpuid/windows/osal_cpuid.c
  @brief   Get unique CPU or computer identifier, windows implementation.
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
#if OSAL_CPUID_SUPPORT
#include <intrin.h>

/* Forward referred static functions.
 */
static void osal_xor_helper(
    unsigned int x,
    os_int *pos,
    os_uchar *buf,
    os_memsz buf_sz);


/**
****************************************************************************************************

  @brief Merge CPU identifier to buffer with XOR.

  The function merges CPU/computer identifier with buffer.

  @param   buf Pointer to buffer to be modified.
  @param   buf_sz Buffer size in bytes.

****************************************************************************************************
*/
osalStatus osal_xor_cpuid(
    os_uchar *buf,
    os_memsz buf_sz)
{
    int cpuinfo[4];
    os_int pos = 0;

    __cpuid(cpuinfo, 0);
    osal_xor_helper(cpuinfo[0], &pos, buf, buf_sz);
    osal_xor_helper(cpuinfo[1], &pos, buf, buf_sz);
    osal_xor_helper(cpuinfo[2], &pos, buf, buf_sz);
    osal_xor_helper(cpuinfo[3], &pos, buf, buf_sz);

    __cpuid(cpuinfo, 1);
    cpuinfo[0] |= 0b11000000000001111;
    osal_xor_helper(cpuinfo[0], &pos, buf, buf_sz);
    osal_xor_helper(cpuinfo[2], &pos, buf, buf_sz);
    osal_xor_helper(cpuinfo[3], &pos, buf, buf_sz);

    __cpuid(cpuinfo, 0x80000001);
    osal_xor_helper(cpuinfo[2], &pos, buf, buf_sz);
    osal_xor_helper(cpuinfo[3], &pos, buf, buf_sz);

    for (i = 0x80000002; i <= 0x80000004; i++) {
        __cpuid(cpuinfo, i);
        osal_xor_helper(cpuinfo[0], &pos, buf, buf_sz);
        osal_xor_helper(cpuinfo[1], &pos, buf, buf_sz);
        osal_xor_helper(cpuinfo[2], &pos, buf, buf_sz);
        osal_xor_helper(cpuinfo[3], &pos, buf, buf_sz);
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Helper function to xor unsigned integer into current buffer position.

  @param   x Inteer value to be xorred in.
  @param   pos Pointer to current position.
  @param   buf Pointer to buffer to be modified.
  @param   buf_sz Buffer size in bytes.

****************************************************************************************************
*/
static void osal_xor_helper(
    int x,
    os_int *pos,
    os_uchar *buf,
    os_memsz buf_sz)
{
    os_int count, i;
    os_uchar *p;

    count = sizeof(x);
    i = *pos;
    p = (os_uchar *)&x;
    while (count--) {
        buf[i++] ^= *(p++);
        if (i >= buf_sz) i = 0;
    }

    *pos = i;
}

#endif
