/**

  @file    int64x/common/osal_int64_double.c
  @brief   Conversions between 64 bit integers and floating point values.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Conversions between 64 bit integers and double precision floating point values.
  Define OSAL_LONG_IS_64_BITS controls compiler's code generation for 64 bit integer arithmetic. 
  If the define is nonzero, then compiler supported 64 bit arithmetic is used. If the define
  is zero, the functions implemented in osal_int64.h are used.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"


/* We need the code from this file only if compiler doesn't support 64 bit integers.
 */
#if OSAL_LONG_IS_64_BITS == 0
#if OSAL_INT64X_SUPPORT


/** 0x100000000 as double. Used to convert between 64 bit integers and double precision 
    floating point numbers.
 */
#define OSAL_INT64_HIGH_DWORD_DIV 4294967296.0


/**
****************************************************************************************************

  @brief Set os_double value to 64 bit integer.
  @anchor osal_int64_set_double

  The osal_int64_set_double() function stores double precision floating point value of type 
  os_double into 64 bit integer. The value is rounded to nearest matching integer.
  This function implementation is used if OSAL_LONG_IS_64_BITS define is zero. If the
  OSAL_LONG_IS_64_BITS define is nonzero, then the os_int64 is the same as os_long, 
  and macro implementation is used instead of this function.

  @param   x Pointer to 64 bit integer to set. 
  @param   v Floating point value to set.

  @return  None.

****************************************************************************************************
*/
void osal_int64_set_double(
    os_int64 *x, 
    os_double v)
{
    os_uint
        v_high;

    os_boolean 
        negative_v;

    negative_v = (os_boolean)(v < 0.0);
    if (negative_v) v = -v;
    v += 0.5;

    v_high = (os_uint)(v / OSAL_INT64_HIGH_DWORD_DIV);
    v -= OSAL_INT64_HIGH_DWORD_DIV * v_high;
    if (v > OSAL_INT64_HIGH_DWORD_DIV-0.5) v = OSAL_INT64_HIGH_DWORD_DIV-0.5;
    if (v < 0.0) v = 0.0;

    osal_int64_set_uint2(x, (os_uint)v, v_high);
    if (negative_v) osal_int64_negate(x);
}


/**
****************************************************************************************************

  @brief Get value of 64 bit integer as os_double.
  @anchor osal_int64_get_double

  The osal_int64_get_double() function gets value of 64 bit integer as os_double, double
  precision floating point number.
  This function implementation is used if OSAL_LONG_IS_64_BITS define is zero. If the
  OSAL_LONG_IS_64_BITS define is nonzero, then the os_int64 is the same as os_long, 
  and macro implementation is used instead of this function.

  @param   x Pointer to 64 bit integer. 

  @return  Value converted to floating point.

****************************************************************************************************
*/
os_double osal_int64_get_double(
    os_int64 *x)
{
    os_uint
        v_low, 
        v_high;

    os_int64 
        y;

    os_double 
        v;

    os_boolean 
        negative_v;

    osal_int64_copy(&y, x);
    negative_v = osal_int64_is_negative(&y);
    if (negative_v) osal_int64_negate(&y);

    osal_int64_get_uint2(&y, &v_low, &v_high);

    v = OSAL_INT64_HIGH_DWORD_DIV * (os_double)v_high + (os_double)v_low;
    return negative_v ? -v : v;
}


/* End OSAL_INT64X_SUPPORT and OSAL_LONG_IS_64_BITS == 0
 */
#endif
#endif
