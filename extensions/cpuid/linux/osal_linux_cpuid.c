/**

  @file    cpuid/linux/osal_cpuid.c
  @brief   Get unique CPU or computer identifier, linux implementation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  On linux one can include cpuid.h which declares these functions:

  Return highest supported input value for cpuid instruction.  ext can be either 0x0 or
  0x8000000 to return highest supported value for basic or extended cpuid information.
  Function returns 0 if cpuid is not supported or whatever cpuid returns in eax register.
  If sig pointer is non-null, then first four bytes of the signature (as found in ebx
  register) are returned in location pointed by sig.

        unsigned int __get_cpuid_max(
            unsigned int __ext,
            unsigned int *__sig)

  Return cpuid data for requested cpuid level, as found in returned eax, ebx, ecx and edx
  registers.  The function checks if cpuid is supported and returns 1 for valid cpuid
  information or 0 for unsupported cpuid level.  All pointers are required to be non-null.

        int __get_cpuid(
            unsigned int __level,
            unsigned int *__eax, unsigned int *__ebx,
            unsigned int *__ecx, unsigned int *__edx)

  Simple test code:

        os_uchar buf[52];
        os_int i;
        os_ulong sum = 0;
        os_memclear(buf, sizeof(buf));
        osalStatus ss = osal_xor_cpuid(buf, sizeof(buf));
        for (i = 0; i<sizeof(buf); i++) {
            sum += buf[i];
        }


  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_LINUX
#if OSAL_CPUID_SUPPORT
#include "cpuid.h"

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
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0, i;
    os_int pos = 0;

    if (__get_cpuid (0, &eax, &ebx, &ecx, &edx) == -1) {
        return OSAL_STATUS_FAILED;
    }
    osal_xor_helper(eax, &pos, buf, buf_sz);
    osal_xor_helper(ebx, &pos, buf, buf_sz);
    osal_xor_helper(ecx, &pos, buf, buf_sz);
    osal_xor_helper(edx, &pos, buf, buf_sz);

    if (__get_cpuid (1, &eax, &ebx, &ecx, &edx) == -1) {
        return OSAL_STATUS_FAILED;
    }
    eax |= 0b11000000000001111;
    osal_xor_helper(eax, &pos, buf, buf_sz);
    osal_xor_helper(ecx, &pos, buf, buf_sz);
    osal_xor_helper(edx, &pos, buf, buf_sz);

    if (__get_cpuid (0x80000001, &eax, &ebx, &ecx, &edx) == -1) {
        return OSAL_STATUS_FAILED;
    }
    osal_xor_helper(ecx, &pos, buf, buf_sz);
    osal_xor_helper(edx, &pos, buf, buf_sz);

    for (i = 0x80000002; i <= 0x80000004; i++) {
        if (__get_cpuid (0, &eax, &ebx, &ecx, &edx) == -1) {
            return OSAL_STATUS_FAILED;
        }
        osal_xor_helper(eax, &pos, buf, buf_sz);
        osal_xor_helper(ebx, &pos, buf, buf_sz);
        osal_xor_helper(ecx, &pos, buf, buf_sz);
        osal_xor_helper(edx, &pos, buf, buf_sz);
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
    unsigned int x,
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
#endif
