/**

  @file    rand/arduino/osal_rand.c
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

  @param   x Seed value.
  @return  None.

****************************************************************************************************
*/
void osal_rand_seed(
    os_long x)
{
    os_timer z;
    os_get_timer(&z);
    randomSeed((unsigned long)(x ^ z));
}


/**
****************************************************************************************************

  @brief Get a pseudo random number.
  @anchor osal_rand

  The osal_rand() function returns random number from min_value to max_value (inclusive).
  All possible retuned values have same propability.

  @param   min_value Minimum value for random number.
  @param   max_value Maximum value for random number.
  @return  Random number from min_value to max_value.

****************************************************************************************************
*/
os_long osal_rand(
    os_long min_value,
    os_long max_value)
{
    os_long x, z, range;
    os_timer t;

    range = max_value - min_value + 1;

    x = random(-2147483648, 2147483647);
    z = random(-2147483648, 2147483647);
    x ^= z << 32;
    os_get_timer(&t);
    x ^= t;

    return min_value + x % range;
}

#endif
