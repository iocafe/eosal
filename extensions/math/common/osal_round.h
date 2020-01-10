/**

  @file    extensions/math/common/osal_round.h
  @brief   Macros for rounding floating point numbers to integers.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

#define os_round_short(d) ((d)>=0.0 ? (os_short)((d) + 0.5) : -(os_short)((-(d)) + 0.5))
#define os_round_int(d) ((d)>=0.0 ? (os_int)((d) + 0.5) : -(os_int)((-(d)) + 0.5))
#define os_round_long(d) ((d)>=0.0 ? (os_long)((d) + 0.5) : -(os_long)((-(d)) + 0.5))
