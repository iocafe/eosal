/**

  @file    common/osal_rand.h
  @brief   Get random number.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    25.12.2016

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_RAND_INCLUDED
#define OSAL_RAND_INCLUDED
#if OSAL_RAND_SUPPORT

/** 
****************************************************************************************************

  @name Generate random numbers

  The osal_rand() function returns random number from min_value to max_value (inclusive).
  All possible retuned values have same propability.

****************************************************************************************************
 */
/*@{*/

/* Get a pseudo random number.
 */
os_long osal_rand(
    os_long min_value,
    os_long max_value);

/*@}*/

#endif
#endif
