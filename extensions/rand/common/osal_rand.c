/**

  @file    rand/common/osal_rand.c
  @brief   Get random number.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Currently returns just C standard library pseudo random numbers. This implementation is very
  weak for encryprion. Better platform specific implementations are to be done in future.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_RAND_SUPPORT == OSAL_RAND_COMMON

#include <stdlib.h>


// USE Hash_DRBG with OPENSSL

/**
****************************************************************************************************

  @brief Set pseudo random number generator seed.
  @anchor osal_rand_seed

  The osal_rand_seed() function sets pseudo random number generator seed value.

  @param   ent Entropy (from physical random source) to seed the random number generator.
  @param   ent_sz Entropy size in bytes.
  @return  None.

****************************************************************************************************
*/
void osal_rand_seed(
    const os_char *ent,
    os_memsz ent_sz)
{
    os_char *p;
    os_timer z;
    os_short i, max_sz;

    os_get_timer(&z);
    p = (os_char*)&z;
    max_sz = sizeof(z);
    if (ent_sz > max_sz) max_sz = (os_short)ent_sz;
    for (i = 0; i < max_sz; i++)
    {
        p[i % sizeof(z)] ^= ent[i % ent_sz];
    }

    srand((unsigned int)z);
}


/**
****************************************************************************************************

  @brief Get a pseudo random number.
  @anchor osal_rand

  The osal_rand() function returns random number from min_value to max_value (inclusive).
  All possible retuned values have same propability.

  @param   min_value Minimum value for random number.
  @param   max_value Maximum value for random number.
  @return  Random number from min_value to max_value. If min_value equals max_value, all
           64 bits of return value are random data.

****************************************************************************************************
*/
os_long osal_rand(
    os_long min_value,
    os_long max_value)
{
    os_long range, x, z;

    os_get_timer(&x);
    z = rand();
    x ^= z;
    z = rand();
    x ^= z << 14;
    z = rand();
    x ^= z << 28;
    z = rand();
    x ^= z << 42;
    z = rand();
    x ^= z << 56;

    if (max_value == min_value) return x;
    range = max_value - min_value + 1;
    return min_value + (os_long)((os_ulong)x % (os_ulong)range);
}

#endif
