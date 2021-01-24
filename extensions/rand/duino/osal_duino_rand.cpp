/**

  @file    rand/arduino/osal_rand_arduino.c
  @brief   Get random number.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  In future this can be replaced with better alternative.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_RAND_SUPPORT == OSAL_RAND_PLATFORM

#include <Arduino.h>


/**
****************************************************************************************************

  @brief Set pseudo random number generator seed.
  @anchor osal_rand_seed

  The osal_rand() function returns random number from min_value to max_value (inclusive).
  All possible retuned values have same propability.

  Arduino specific: 32 bits used.

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
    os_int i, max_sz;

    os_get_timer(&z);
    p = (os_char*)&z;
    max_sz = sizeof(z);
    if (ent_sz > max_sz) max_sz = (os_int)ent_sz;
    for (i = 0; i < max_sz; i++)
    {
        p[i % sizeof(z)] ^= ent[i % ent_sz];
    }

    randomSeed((unsigned long)z);
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
    os_long x, z, range;
    os_timer t;

    x = random(-2147483648, 2147483647);
    z = random(-2147483648, 2147483647);
#if OSAL_LONG_IS_64_BITS
    x ^= z << 32;
#else
    x ^= z << 5;
#endif
    os_get_timer(&t);
    x ^= t;

    if (max_value == min_value) return x;
    range = max_value - min_value + 1;
    return min_value + (os_long)((os_ulong)x % (os_ulong)range);
}

#endif
