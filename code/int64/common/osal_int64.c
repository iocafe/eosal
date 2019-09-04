/**

  @file    int64/commmon/osal_int64.c
  @brief   64 bit integer arithmetic.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Basic 64 bit integer functions. Setting, getting and comparing values. Addition and substraction.
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


/* 64 bit integer constants.
 */
#if OSAL_LONG_IS_64_BITS

/** 64 bit constant global variable holding value 1. Do not modify the value.
 */
const os_int64 osal_int64_1 = 1;

/** 64 bit constant global variable holding value 10. Do not modify the value.
 */
const os_int64 osal_int64_10 = 10;

/** 64 bit constant global variable holding value 1000. Do not modify the value.
 */
const os_int64 osal_int64_1000 = 1000;


/** 64 bit constant global variable holding value 1000000. Do not modify the value.
 */
const os_int64 osal_int64_1000000 = 1000000;

#else

/** 64 bit constant global variable holding value 1. Do not modify the value.
 */
const os_int64 osal_int64_1 = {{1, 0, 0, 0}};

/** 64 bit constant global variable holding value 10. Do not modify the value.
 */
const os_int64 osal_int64_10 = {{10, 0, 0, 0}};

/** 64 bit constant global variable holding value 1000. Do not modify the value.
 */
const os_int64 osal_int64_1000 = {{1000, 0, 0, 0}};

/** 64 bit constant global variable holding value 1000000. Do not modify the value.
 */
const os_int64 osal_int64_1000000 = {{0x4240, 0x000F, 0, 0}};

#endif


/* We need the code from this file only if compiler doesn't support 64 bit integers.
 */
#if OSAL_LONG_IS_64_BITS == 0


/**
****************************************************************************************************

  @brief Set os_long value to 64 bit integer.
  @anchor osal_int64_set_long

  The osal_int64_set_long() function stores integer value of type os_long into 64 bit 
  integer. Notice that os_long is either 32 or 64 bits, depending on compiler support and 
  OSAL_LONG_IS_64_BITS define.
  This function implementation is used if OSAL_LONG_IS_64_BITS define is zero. If the
  OSAL_LONG_IS_64_BITS define is nonzero, then the os_int64 is the same as os_long, 
  and macro implementation is used instead of this function.

  @param   x Pointer to 64 bit integer to set. 
  @param   v Integer value to set.

  @return  None.

****************************************************************************************************
*/
void osal_int64_set_long(
    os_int64 *x, 
    os_long v)
{
#if OSAL_SMALL_ENDIAN
    x->dw[0] = (os_uint)v;
    x->dw[1] = v < 0 ? 0xFFFFFFFF : 0;
#else
    x->w[0] = (os_ushort)v;
    x->w[1] = (os_ushort)(v >> 16);
    x->dw[1] = v < 0 ? 0xFFFFFFFF : 0;
#endif
}


/**
****************************************************************************************************

  @brief Make 64 bit integer from two 32 bit integers.
  @anchor osal_int64_set_uint2

  The osal_int64_set_uint2() function sets a 64 bit integer from 32 bit low and high parts.
  This function implementation is used if OSAL_LONG_IS_64_BITS define is zero. If the
  OSAL_LONG_IS_64_BITS define is nonzero, then the os_int64 is the same as os_long, 
  and macro implementation is used instead of this function.

  @param   x Pointer to 64 bit integer to set. 
  @param   v_low Less significant 32 bits.
  @param   v_high More significant 32 bits.

  @return  None.

****************************************************************************************************
*/
void osal_int64_set_uint2(
    os_int64 *x, 
    os_uint v_low, 
    os_uint v_high)
{
#if OSAL_SMALL_ENDIAN
    x->dw[0] = v_low;
    x->dw[1] = v_high;
#else
    x->w[0] = (os_ushort)v_low;
    x->w[1] = (os_ushort)(v_low >> 16);
    x->w[2] = (os_ushort)v_high;
    x->w[3] = (os_ushort)(v_high >> 16);
#endif
}


/**
****************************************************************************************************

  @brief Get value of a 64 bit integer as os_long.
  @anchor osal_int64_get_long

  The osal_int64_get_long() function gets value of 64 bit integer as os_long. Notice that
  os_long is either 32 or 64 bits, depending on compiler support and OSAL_LONG_IS_64_BITS
  define.
  This function implementation is used if OSAL_LONG_IS_64_BITS define is zero. If the
  OSAL_LONG_IS_64_BITS define is nonzero, then the os_int64 is the same as os_long, 
  and macro implementation is used instead of this function.

  @param   x Pointer to 64 bit integer. 

  @return  Integer value.

****************************************************************************************************
*/
os_long osal_int64_get_long(
    os_int64 *x)
{
#if OSAL_SMALL_ENDIAN
    return x->dw[0];
#else
    return (os_long)(((os_uint)x->w[0]) | (((os_uint)x->w[1]) << 16))
#endif
}


/**
****************************************************************************************************

  @brief Break 64 bit integer into two 32 bit integers.
  @anchor osal_int64_get_uint2

  The osal_int64_get_uint2() function sets value of 64 bit integer as two 32 bit integers,
  low and high parts.
  This function implementation is used if OSAL_LONG_IS_64_BITS define is zero. If the
  OSAL_LONG_IS_64_BITS define is nonzero, then the os_int64 is the same as os_long, 
  and macro implementation is used instead of this function.

  @param   x Pointer to 64 bit integer. 
  @param   v_low Pointer where to store less significant 32 bits.
  @param   v_high Pointer where to store more significant 32 bits.

  @return  None.

****************************************************************************************************
*/
void osal_int64_get_uint2(
    os_int64 *x, 
    os_uint *v_low, 
    os_uint *v_high)
{
#if OSAL_SMALL_ENDIAN
    *v_low = x->dw[0];
    *v_high = x->dw[1];
#else
    *v_low = (os_uint)(((os_ushort)x->w[0]) | (((os_ushort)x->w[1]) << 16));
    *v_high = (os_uint)(((os_ushort)x->w[2]) | (((os_ushort)x->w[3]) << 16));
#endif
}


/**
****************************************************************************************************

  @brief Add two 64 bit integers.
  @anchor osal_int64_add

  The osal_int64_add() function adds two 64 bit integers, x and y, together and stores the
  result into x.
  This function implementation is used if OSAL_LONG_IS_64_BITS define is zero. If the
  OSAL_LONG_IS_64_BITS define is nonzero, then the os_int64 is the same as os_long, 
  and macro implementation is used instead of this function.

  @param   x Pointer to the first 64 bit integer, result is stored here. 
  @param   y Pointer to the second 64 bit integer, remains unmodified. 

  @return  OS_TRUE (1) if carry bit, OS_FALSE (0) otherwise. Avoid using this return value:
           The macro version (used when compiler spports 64 bit integers) will not return value.

****************************************************************************************************
*/
os_boolean osal_int64_add(
    os_int64 *x, 
    const os_int64 *y)
{
    os_uint 
        v;

    const os_ushort
        *yw;
	
	os_ushort
        *xw,
        count;

    xw = x->w;
    yw = y->w;
    
    count = 4;
    v = 0;
    while (count--)
    {
        v += (os_uint)*xw + (os_uint)*(yw++);
        *(xw++) = (os_ushort)v;
        v >>= 16;
    }

    return (os_boolean)v;
}


/**
****************************************************************************************************

  @brief Subtract 64 bit integer from another 64 bit integer.
  @anchor osal_int64_subtract

  The osal_int64_subtract() function subtracts y from x and stores the result into x.
  This function implementation is used if OSAL_LONG_IS_64_BITS define is zero. If the
  OSAL_LONG_IS_64_BITS define is nonzero, then the os_int64 is the same as os_long, 
  and macro implementation is used instead of this function.

  @param   x Pointer to the first 64 bit integer, result is stored here. 
  @param   y Pointer to the 64 bit integer su subtract, remains unmodified. 

  @return  OS_TRUE (1) if carry bit, OS_FALSE (0) otherwise. Avoid using this return value:
           The macro version (used when compiler spports 64 bit integers) will not return value.

****************************************************************************************************
*/
os_boolean osal_int64_subtract(
    os_int64 *x, 
    const os_int64 *y)
{
    os_uint
        borrow,
        next_borrow;

    os_ushort
        *xw,
        count;

    const os_ushort
        *yw;

    xw = x->w;
    yw = y->w;

    count = 4;
    borrow = 0;
    while (count--)
    {
        next_borrow = (*yw + borrow > *xw);
        *xw = (os_ushort)(*xw - *(yw++) - borrow);
        xw++;
		borrow = next_borrow;
    }

    return (os_boolean)next_borrow;
}


/**
****************************************************************************************************

  @brief Negate 64 bit integer.
  @anchor osal_int64_divide

  The osal_int64_negate() function negates 64 bit integer x. This is simple one's complement.
  This function implementation is used if OSAL_LONG_IS_64_BITS define is zero. If the
  OSAL_LONG_IS_64_BITS define is nonzero, then the os_int64 is the same as os_long, 
  and macro implementation is used instead of this function.

  @param   x Pointer to the 64 bit integer, result is stored here. 

  @return  None.

****************************************************************************************************
*/
void osal_int64_negate(
    os_int64 *x)
{
    x->dw[0] = ~x->dw[0];
    x->dw[1] = ~x->dw[1];

    /* The code below does the same as osal_int64_add(x, &osal_int64_1), just a little bit
	   faster
	 */
	if (x->w[0] != 0xFFFF)
	{
		x->w[0]++;
	}
	else
	{ 
		osal_int64_add(x, &osal_int64_1);
	}
}


/**
****************************************************************************************************

  @brief Compare two 64 bit integers.
  @anchor osal_int64_compare

  The osal_int64_compare() function compares two 64 bit integers, x and y. If x is greater
  than y, then the function returns 1. If x is less than y, then function returns -1.
  If x and y are equal, the function returns 0. This is always signed compare.
  This function implementation is used if OSAL_LONG_IS_64_BITS define is zero. If the
  OSAL_LONG_IS_64_BITS define is nonzero, then the os_int64 is the same as os_long, 
  and macro implementation is used instead of this function.

  @param   x Pointer to the first 64 bit integer, remains unmodified. 
  @param   y Pointer to the second 64 bit integer, remains unmodified. 

  @return  1 if x > y, -1 if x < y, 0 if x == y.

****************************************************************************************************
*/
os_int osal_int64_compare(
    const os_int64 *x, 
    const os_int64 *y)
{
    /* If more significant half is same, same signs
     */
    if (x->dw[1] == y->dw[1]) 
    {
        if (x->dw[0] == y->dw[0]) return 0;
#if OSAL_SMALL_ENDIAN
        return x->dw[0] > y->dw[0] ? 1 : -1;
#else
        if (x->w[1] == y->w[1]) 
        {
            return x->w[0] > y->w[0] ? 1 : -1;
        }
        return x->w[1] > y->w[1] ? 1 : -1;
#endif
    }
#if OSAL_SMALL_ENDIAN
    /* Signed compare
     */
    return ((os_int)x->dw[1] > (os_int)y->dw[1]) ? 1 : -1;
#else
    /* If most significant word is same, same signs.
     */
    if (x->w[3] == y->w[3]) 
    {
        return x->w[2] > y->w[2] ? 1 : -1;
    }

    /* Use signed compare.
     */
    return (os_short)x->w[3] > (os_short)y->w[3] ? 1 : -1;
#endif
}


/* End OSAL_LONG_IS_64_BITS == 0
 */
#endif
