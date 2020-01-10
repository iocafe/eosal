/**

  @file    rand/common/osal_rand.c
  @brief   Get random number.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

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


/**
****************************************************************************************************

  @brief Set pseudo random number generator seed.
  @anchor osal_rand_seed

  The osal_rand_seed() function sets pseudo random number generator seed value.

  @param   x Seed value.
  @return  None.

****************************************************************************************************
*/
void osal_rand_seed(
    os_long x)
{
    os_timer z;
    os_get_timer(&z);
    srand((unsigned int)(x ^ z));
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
    os_long range, x, z;

#if OSAL_DEBUG
    static os_boolean range_error_reported = OS_FALSE;
#endif

    range = max_value - min_value + 1;

    if (range <= 0)
    {
#if OSAL_DEBUG
        if (!range_error_reported)
        {
            osal_debug_error("osal_rand: Range cannot be used");
            range_error_reported = OS_TRUE;
        }
#endif
        range = 0x1000;
    }

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

    return min_value + x % range;
}

#endif
