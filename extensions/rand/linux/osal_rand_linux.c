/**

  @file    rand/linux/osal_rand_linux.c
  @brief   Get random number.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    20.1.2020

  Uses Linux system function getrandom to generate random numbers. This is based on linux
  /dev/urandom, which is seeded by entropy colledted by operating system and does provide
  strong random number.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_RAND_SUPPORT == OSAL_RAND_PLATFORM

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <unistd.h>
#include <sys/syscall.h>
#include "linux/random.h"


/**
****************************************************************************************************

  @brief Set pseudo random number generator seed.
  @anchor osal_rand_seed

  The osal_rand_seed() function is not needed in linux. The kernel device /dev/urandom collects
  the entropy. Function is provided just to allow linux build of code which tries to seed the
  random generator.

  @param   ent Entropy (from physical random source) to seed the random number generator.
  @param   ent_sz Entropy size in bytes.
  @return  None.

****************************************************************************************************
*/
void osal_rand_seed(
    const os_char *ent,
    os_memsz ent_sz)
{
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
    os_long range, x;
    os_timer r;

    r = syscall(SYS_getrandom, &x, sizeof(x), 0);
    if (r == -1)
    {
        os_get_timer(&r);
        x = r;
        osal_debug_error("osal_rand() failed");
    }

    if (max_value == min_value) return x;
    range = max_value - min_value + 1;
    return min_value + (os_long)((os_ulong)x % (os_ulong)range);
}

#endif
