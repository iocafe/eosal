/**

  @file    int64x/common/osal_int64_multiply.c
  @brief   64 bit integer arithmetic, multiplication.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Implements 64 bit multiplication on platforms with 32 bit multiplication.

  Define OSAL_LONG_IS_64_BITS controls compiler's code generation for 64 bit integer arithmetic. 
  If the define is nonzero, then compiler supported 64 bit arithmetic is used. If the define
  is zero, the functions implemented in osal_int64.h are used.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
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


/**
****************************************************************************************************

  @brief Multiply two 64 bit integers.
  @anchor osal_int64_multiply

  The osal_int64_multiply() function multiplies two 64 bit integers, x and y, and stores the
  result into x.
  This function implementation is used if OSAL_LONG_IS_64_BITS define is zero. If the
  OSAL_LONG_IS_64_BITS define is nonzero, then the os_int64 is the same as os_long, 
  and macro implementation is used instead of this function.

  @param   x Pointer to the first 64 bit integer, result is stored here. 
  @param   y Pointer to the second 64 bit integer, remains unmodified. 

  @return  None.

****************************************************************************************************
*/
void osal_int64_multiply(
    os_int64 *x, 
    const os_int64 *y)
{
    os_int64
        yy;

    os_boolean
        negative_result;

    negative_result = OS_FALSE;
    if (osal_int64_is_negative(x)) 
    {
        osal_int64_negate(x);
        negative_result = OS_TRUE;
    }

    if (osal_int64_is_negative(y)) 
    {
        osal_int64_copy(&yy, y);
        osal_int64_negate(&yy);
        y = &yy;
        negative_result = !negative_result;
    }

	osal_int64_unsigned_multiply(x, y, x);

    if (negative_result) osal_int64_negate(x);
}


/**
****************************************************************************************************

  @brief Multiply two unsigned 64 bit integers.

  The osal_int64_unsigned_multiply() function multiplies two 64 bit integers, x and y, and stores
  the result. This is internal helper function for the 64 bit integer arithmetic.

  @param   x Pointer to the first 64 bit integer, result is stored here. 
  @param   y Pointer to the second 64 bit integer, remains unmodified. 
  @param   result Pointer to 64 bit integer into which to store the result.

  @return  None.

****************************************************************************************************
*/
void osal_int64_unsigned_multiply(
    const os_int64 *x, 
    const os_int64 *y,
	os_int64 *result) 
{
    os_uint 
        a, 
        b,
        u;

    os_int64 
        tmp,
		sum;

    os_ushort
        i,
        j,
        c0,
		ywi;

	const os_ushort
		*xw,
		*yw;

	xw = x->w;
	yw = y->w;

	osal_int64_set_zero(&sum);

    for (i = 0; i < 4; i++)
    {
		ywi = yw[i];
		if (ywi == 0) continue;

		osal_int64_set_zero(&tmp);
        a = (os_uint)ywi * (os_uint)xw[0];
        tmp.w[i] = (os_ushort)a;
        a >>= 16;

        for (j = 1; j < 4 - i; j++)
        {
            b = (os_uint)ywi * (os_uint)xw[j];

            u = (os_uint)(a & 0xFFFF) + (os_uint)(b & 0xFFFF);
            c0 = (os_ushort)u;
            u >>= 16;
            u += (a>>16) + (b>>16);
            a = u;

            tmp.w[i+j] = c0;
        }

        /* The code below does the same as osal_int64_add(&sum, &tmp), 
		   rewritten just to inline.
		 */
		u = 0;
		for (j = i; j < 4; ++j)
		{
			u += (os_uint)sum.w[j] + (os_uint)tmp.w[j];
			sum.w[j] = (os_ushort)u;
			u >>= 16;
		}
    }

    osal_int64_copy(result, &sum);
}

/* End OSAL_INT64X_SUPPORT and OSAL_LONG_IS_64_BITS == 0
 */
#endif
#endif
