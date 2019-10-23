/**

  @file    serialize/linux/osal_float.c
  @brief   Convert floating point number to two integers and vice versa.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"


/**
****************************************************************************************************

  @brief Convert integer mantissa and exponent to double.
  @anchor osal_ints2double

  The osal_ints2double() function takes mantissa and exponent as arguments converts these to
  a double precision floating point number.

  @param  x Pointer to double in which to store the result.
  @param  m Mantissa.
  @param  e Exponent.

  @return OS_TRUE if successfull, OS_FALSE if overflow or another conversion error.

****************************************************************************************************
*/
os_boolean osal_ints2double(
    os_double *x,
    os_long m,
    os_long e)
{
    os_long sign;
    os_boolean rval;

    union
    {
        os_long l;
        os_double d;
    } mu;

    mu.l = m;

    /* If mantissa is zero, the floating point value is zero.
     */
    if (mu.l == 0)
    {
        *x = 0.0;
        return OS_TRUE;
    }

    /* If mantissa is negative, set sign flag and turn mantissa to positive.
     */
    if (mu.l < 0)
    {
        sign = 0x8000000000000000;
        mu.l = -mu.l;
    }
    else
    {
        sign = 0;
    }

    /* Assume success as return value.
     */
    rval = OS_TRUE;

    /* If underflow, use smallest possible value.
     */
    if (e < -1023)
    {
        e = -1023;
        mu.l = 0x10000000000000;
    }

    /* If overflow, use biggest possible value and set return
       value to failed.
     */
    if (e > 1023)
    {
        e = 1023;
        mu.l = 0x1FFFFFFFFFFFFF;
        rval = OS_FALSE;
    }

    /* Move leading 1 to right position.
     */
    while (mu.l >= 0x20000000000000) mu.l >>= 1;
    while (mu.l < 0x10000000000000) mu.l <<= 1;

    /* Merge exponent and sign into mantissa.
     */
    e += 1023;
    mu.l |= ((os_long)e << 52) | sign;

    /* Return the result.
     */
    *x = mu.d;
    return rval;
}


/**
****************************************************************************************************

  @brief Split double to mantissa and exponent.
  @anchor osal_double2ints

  The osal_double2ints() function takes double precision floating point number as argument and
  converts it to integer mantissa and exponent.

  @param  x Number to convert.
  @param  m Pointer where to store mantissa.
  @param  e Pointer where to store exponent.
  @return None.

****************************************************************************************************
*/
void osal_double2ints(
	os_double x,
	os_long *m,
    os_long *e)
{
    os_long sign;

    union
    {
        os_long l;
        os_double d;
    } mu;

    mu.d = x;

    /* If this is zero?
     */
    if (mu.l == 0x10000000000000)
    {
        *m = *e = 0;
        return;
    }

    /* Get sign.
     */
    sign = mu.l & 0x8000000000000000;
    mu.l &= 0x7FFF000000000000;

    /* Return exponent and mantissa
     */
    *e = (mu.l >> 52) - 1023;

    /* Shift right, until rightmost bit is 1. We want the integer mantissa 
       to be as small as possible.
     */
    mu.l &= 0x000FFFFFFFFFFFFF;
    while (mu.l & 1) mu.l >>= 1;

    /* If we need to put sign back?
     */
    if (sign) mu.l = -mu.l;
    *m = mu.l;
}


/**
****************************************************************************************************

  @brief Convert integer mantissa and exponent to float.
  @anchor osal_ints2float

  The osal_ints2float() function takes mantissa and exponent as arguments converts these to
  a single precision floating point number.

  @param  x Pointer to float in which to store the result.
  @param  m Mantissa.
  @param  e Exponent.

  @return OS_TRUE if successfull, OS_FALSE if overflow or another conversion error.

****************************************************************************************************
*/
os_boolean osal_ints2float(
    os_float *x,
    os_long m,
    os_long e)
{
    os_int sign;
    os_boolean rval;

    union
    {
        os_int i;
        os_float f;
    } mu;

    /* If mantissa is zero, the floating point value is zero.
     */
    if (m == 0)
    {
        *x = 0.0;
        return OS_TRUE;
    }

    /* If mantissa is negative, set sign flag and turn mantissa to positive.
     */
    if (m < 0)
    {
        sign = 0x80000000;
        m = -m;
    }
    else
    {
        sign = 0;
    }

    /* Assume success as return value.
     */
    rval = OS_TRUE;

    /* If underflow, use smallest possible value.
     */
    if (e < -127)
    {
        e = -127;
        m = 0x800000;
    }

    /* If overflow, use biggest posible value and set return.
       value to failed.
     */
    if (e > 127)
    {
        e = 127;
        m = 0xFFFFFF;
        rval = OS_FALSE;
    }

    /* Move leading 1 to right position.
     */
    while (m >= 0x1000000) m >>= 1;
    while (m < 0x800000) m <<= 1;

    /* Merge exponent and sign into mantissa.
     */
    e += 127;
    mu.i = (os_int)m | ((os_int)e << 23) | sign;

    /* Return the result.
     */
    *x = mu.f;
    return rval;
}


/**
****************************************************************************************************

  @brief Split float to mantissa and exponent.
  @anchor osal_float2ints

  The osal_float2ints() function takes single precision floating point number as argument and
  converts it to integer mantissa and exponent.

  @param  x Number to convert.
  @param  m Pointer where to store mantissa.
  @param  e Pointer where to store exponent.
  @return None.

****************************************************************************************************
*/
void osal_float2ints(
    os_float x,
    os_long *m,
    os_long *e)
{
    os_int sign;

    union
    {
        os_int i;
        os_float f;
    } mu;

    mu.f = x;

    /* If this is zero?
     */
    if (mu.i == 0x800000)
    {
        *m = *e = 0;
        return;
    }

    /* Get sign.
     */
    sign = mu.i &0x80000000;
    mu.i &= 0x7FFF0000;

    /* Return exponent and mantissa
     */
    *e = (mu.i >> 23) - 127;

    /* Shift right, until rightmost bit is 1. We want the integer mantissa 
       to be as small as possible.
     */
    mu.i &= 0x007FFFFF;
    while (mu.i & 1) mu.i >>= 1;

    /* If we need to put sign back?
     */
    if (sign) mu.i = -mu.i;
    *m = mu.i;
}
