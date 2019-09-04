/**

  @file    rand/common/osal_rand.c
  @brief   Get random number.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    25.12.2016

  Currently returns just C standard library pseudo random numbers. In future this can be replaced
  with better alternative.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_RAND_SUPPORT

#include <stdlib.h>


/**
****************************************************************************************************

  @brief Get a pseudo random number.
  @anchor osal_rand

  The osal_rand() function returns random number from min_value to max_value (inclusive).
  All possible retuned values have same propability.

  @param   min_value Minimum value for random number.
  @param   max_value Maximum value for random number.
  @return  None.

****************************************************************************************************
*/
os_long osal_rand(
    os_long min_value,
    os_long max_value)
{
    return min_value + rand() % (max_value - min_value + 1);
}

#endif
