/**

  @file    int64/common/osal_int64.h
  @brief   64 bit integer arithmetic.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020


  Define OSAL_COMPILER_HAS_64_BIT_INTS controls compiler's code generation for 64 bit integer arithmetic.
  If the define is nonzero, then compiler supported 64 bit arithmetic is used. If the define
  is zero, the functions implemented in osal_int64.h are used.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_INT64_H_
#define OSAL_INT64_H_
#include "eosal.h"

/**
****************************************************************************************************

  @name 64 bit Integer Data Type.

  If compiler supports 64 bit integers, 64 bit integer data type os_int64 is simply same
  as os_long. If there is no compiler support for 64 bit arithmetic, then os_int64 is
  defines as union, which contains w[4] (word) and dw[2] (double work) arrays.

****************************************************************************************************
*/
/*@{*/

#if OSAL_COMPILER_HAS_64_BIT_INTS

/** 64 bit integer type if compiler supports 64 bit integers.
 */
typedef os_longlong os_int64;
  

#else

/** 64 bit integer type if compiler doesn't support 64 bit integers. The union contains
    two arrays, w[4] and dw[2], which are on top of each others. The functions in 
    os_int64.c use these arrays to implement 64 bit arithmetic.
 */
typedef union
{
    /** Array of 4 words, 16 bits each. Lest significant word is the first one w[0] and 
        most significant w[3].
     */
    os_ushort w[4];

    /** Array of 2 double words, 32 bits each. Less significant doublw word is the first 
        one dw[0] and more significant dw[1].
     */
    os_uint dw[2];
}
os_int64;

#endif
/*@}*/



/**
****************************************************************************************************

  @name Constants. 

  Some commonly used 64 bit constants.

****************************************************************************************************
*/
/*@{*/

extern const os_int64 osal_int64_1;
extern const os_int64 osal_int64_10;
extern const os_int64 osal_int64_1000;
extern const os_int64 osal_int64_1000000;

/*@}*/


/**
****************************************************************************************************

  @name 64 bit Integer Arithmetic Macro with Compiler Support. 

  If compiler supports 64 bit integer type (OSAL_COMPILER_HAS_64_BIT_INTS define is nonzero),
  the os_longlong  is 64 bit and os_int64 is same as os_long. The compiler's 64 bit functionality
  is used trough these macros, so that same code will compile also on systems without 64 bit support.

****************************************************************************************************
*/
/*@{*/

#if OSAL_COMPILER_HAS_64_BIT_INTS

/** Set 64 bit integer to zero. Macro implementation to be used if compiler supports 
    64 bit integers.
 */
#define osal_int64_set_zero(x) (*(x) = 0)

/** Copy 64 bit integer. Macro implementation to be used if compiler supports 
    64 bit integers.
 */
#define osal_int64_copy(x,y) (*(x) = *(y))

/** Check if 64 bit integer is zero. The macro returns OS_TRUE (1) if value is zero, or
    OS_FALSE (0) if value is nonzero.
 */
#define osal_int64_is_zero(x) ((os_boolean)(*(x) == 0))

/** Check if 64 bit integer is negative. The macro returns OS_TRUE (1) if value is negative,
    or OS_FALSE (0) if value is positive or zero.
 */
#define osal_int64_is_negative(x) ((os_boolean)(*(x) < 0))

/** Set os_long value to 64 bit integer. Macro implementation to be used if compiler supports 
    64 bit integers.
 */
#define osal_int64_set_long(x,v) (*(x) = (v))

/** Set os_double value to 64 bit integer. Macro implementation to be used if compiler supports 
    64 bit integers.
 */
#define osal_int64_set_double(x,v) (*(x) = (os_int64)(v))

/** Make 64 bit integer from two 32 bit integers. Macro implementation to be used if compiler 
    supports 64 bit integers.
 */
#define osal_int64_set_uint2(x,vl,vh) (*(x) = (os_int64)(vl) + (((os_int64)(vh))<<32))

/** Get value of a 64 bit integer as os_long. Macro implementation to be used if compiler 
    supports  64 bit integers.
 */
#define osal_int64_get_long(x) ((os_long)(*(x)))

/** Get value of 64 bit integer as os_double. Macro implementation to be used if compiler
    supports 64 bit integers.
 */
#define osal_int64_get_double(x) ((os_double)*(x))

/** Break 64 bit integer into two 32 bit integers. Macro implementation to be used if compiler
    supports 64 bit integers.
 */
#define osal_int64_get_uint2(x,vl,vh) (*(vl) = (os_uint)*(x), *(vh) = (os_uint)(*(x)>>32)) 

/** Add two 64 bit integers. Macro implementation to be used if compiler
    supports 64 bit integers.
 */
#define osal_int64_add(x,y) (*(x) += *(y))

/** Subtract 64 bit integer from another 64 bit integer. Macro implementation to be used if 
    compiler supports 64 bit integers.
 */
#define osal_int64_subtract(x,y) (*(x) -= *(y))

/** Multiply two 64 bit integers. Macro implementation to be used if compiler
    supports 64 bit integers.
 */
#define osal_int64_multiply(x,y) (*(x) *= *(y))

/** Divide 64 bit integer by another 64 bit integer. Macro implementation to be used if compiler
    supports 64 bit integers.
 */
#define osal_int64_divide(x,y) (*(x) /= *(y))

/** Negate 64 bit integer (one's complement). Macro implementation to be used if compiler
    supports 64 bit integers.
 */
#define osal_int64_negate(x) (*(x) = -*(x))

/** Compare two 64 bit integers. Macro implementation to be used if compiler
    supports 64 bit integers.
 */
#define osal_int64_compare(x,y) ((os_int)((*(x)<*(y)) ? -1 : (*(x)>*(y))))


#endif
/*@}*/


/**
****************************************************************************************************

  @name 64 Bit Integer Arithmetic Functions and Macros without Compiler Suppport.

  If the used compiler doesn't support 64 integers (OSAL_COMPILER_HAS_64_BIT_INTS define is zero), then
  function implementations and a few macros are used. Here are function prototypes and related
  macros.

****************************************************************************************************
 */
/*@{*/

#if OSAL_COMPILER_HAS_64_BIT_INTS == 0

/** Set 64 bit integer to zero. Macro implementation.
    @anchor osal_int64_set_zero
 */
#define osal_int64_set_zero(x) ((x)->dw[0] = (x)->dw[1] = 0)  

/** Copy 64 bit integer. Macro implementation.
    @anchor osal_int64_copy
 */
#define osal_int64_copy(x, y) ((x)->dw[0] = (y)->dw[0], (x)->dw[1] = (y)->dw[1])

/** Check if 64 bit integer is zero. The macro returns OS_TRUE (1) if value is zero, or
    OS_FALSE (0) if value is nonzero.
    @anchor osal_int64_is_zero
 */
#define osal_int64_is_zero(x) ((os_boolean)((x)->dw[0] == 0 && (x)->dw[1] == 0))

/** Check if 64 bit integer is negative. The macro returns OS_TRUE (1) if value is negative,
    or OS_FALSE (0) if value is positive or zero.
    @anchor osal_int64_is_negative
 */
#define osal_int64_is_negative(x) ((os_boolean)((x)->w[3] >= 0x8000))

/* Set os_long value to 64 bit integer.
 */
void osal_int64_set_long(
    os_int64 *x, 
    os_long v);

/* Set os_double value to 64 bit integer.
 */
void osal_int64_set_double(
    os_int64 *x, 
    os_double v);

/* Make 64 bit integer from two 32 bit integers.
 */
void osal_int64_set_uint2(
    os_int64 *x, 
    os_uint v_low, 
    os_uint v_high);

/* Get value of a 64 bit integer as os_long.
 */
os_long osal_int64_get_long(
    os_int64 *x);

/* Get value of 64 bit integer as os_double.
 */
os_double osal_int64_get_double(
    os_int64 *x);

/* Break 64 bit integer into two 32 bit integers.
 */
void osal_int64_get_uint2(
    os_int64 *x, 
    os_uint *v_low, 
    os_uint *v_high);

/* Add two 64 bit integers.
 */
os_boolean osal_int64_add(
    os_int64 *x, 
    const os_int64 *y);

/* Subtract 64 bit integer from another 64 bit integer.
 */
os_boolean osal_int64_subtract(
    os_int64 *x, 
    const os_int64 *y);

/* Multiply two 64 bit integers.
 */
void osal_int64_multiply(
    os_int64 *x, 
    const os_int64 *y);

/* Multiply two unsigned 64 bit integers (internal).
 */
void osal_int64_unsigned_multiply(
    const os_int64 *x, 
    const os_int64 *y,
	os_int64 *result);

/* Divide 64 bit integer by another 64 bit integer.
 */
void osal_int64_divide(
    os_int64 *x, 
    const os_int64 *y);

/* Negate 64 bit integer (one's complement).
 */
void osal_int64_negate(
	os_int64 *x);

/* Compare two 64 bit integers.
 */
os_int osal_int64_compare(
    const os_int64 *x,
	const os_int64 *y);

#endif

/*@}*/

#endif
