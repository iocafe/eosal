/**

  @file    int64x/common/osal_int64_divide.c
  @brief   64 bit integer arithmetic, division.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Implements 64 bit division on platforms with 32 bit multiplication and division.
  Define OSAL_LONG_IS_64_BITS controls compiler's code generation for 64 bit integer arithmetic. 
  If the define is nonzero, then compiler supported 64 bit arithmetic is used. If the define
  is zero, the functions implemented in osal_int64.h are used.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
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

/* Forward referred static functions.
 */
static void osal_int64_unsigned_divide(
    os_int64 *x, 
    const os_int64 *y);


/**
****************************************************************************************************

  @brief Divide 64 bit integer by another 64 bit integer.
  @anchor osal_int64_divide

  The osal_int64_divide() function divides x by y and stores the result into x.
  This function implementation is used if OSAL_LONG_IS_64_BITS define is zero. If the
  OSAL_LONG_IS_64_BITS define is nonzero, then the os_int64 is the same as os_long, 
  and macro implementation is used instead of this function.

  @param   x Pointer to the first 64 bit integer, result is stored here. 
  @param   y Pointer to the 64 bit integer divisor, remains unmodified. 

  @return  None.

****************************************************************************************************
*/
void osal_int64_divide(
    os_int64 *x, 
    const os_int64 *y)
{
    os_int64 yy;
    os_boolean negative_result;

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

    osal_int64_unsigned_divide(x, y);
    if (negative_result) osal_int64_negate(x);
}


/**
****************************************************************************************************

  @brief Divide unsigned 64 bit integer by another unsigned 64 bit integer.

  The osal_int64_unsigned_divide() function divides x by y and stores the result into x. 
  This is internal helper function for the 64 bit integer arithmetic.

  @param   x Pointer to the first 64 bit integer, result is stored here. 
  @param   y Pointer to the 64 bit integer divisor, remains unmodified. 

  @return  None.

****************************************************************************************************
*/
static void osal_int64_unsigned_divide(
    os_int64 *x, 
    const os_int64 *y)
{
    os_ushort *xw, x_start, y_start, x_end, borrow, next_borrow, xx,yy;
    const os_ushort *yw;
    os_short j;
    os_uint x32, y32, y32_original, res;
    os_int64 result, tmp;

	xw = x->w;
	yw = y->w;

	/* Find index of most significant nonzero word in x.
	 */
	x_start = 3; 
	while (xw[x_start] == 0)
	{
		if (x_start-- == 0) goto return_zero;
	}

	/* Find index of most significant nonzero word in y.
	 */
	y_start = 3; 
	while (yw[y_start] == 0)
	{
		if (y_start-- == 0) goto return_zero;
	}

	if (x_start < y_start) goto return_zero;


	/* If simple 32 bit division.
	 */
	if (x_start <= 1)
	{
#if OSAL_SMALL_ENDIAN
		x->dw[0] = x->dw[0] / y->dw[0];
#else
		x32 = (((os_uint)xw[1]) << 16) + (os_uint)xw[0];
		y32 = (((os_uint)yw[1]) << 16) + (os_uint)yw[0];
		x32 /= y32;
		xw[0] = (os_ushort)x32;
		xw[1] = (os_ushort)(x32 >> 16);
#endif
		return;
	}

	osal_int64_set_zero(&result);

	/* If we have one word divisor
	 */
	if (y_start == 0)
	{
		y32 = yw[0];
		while (1)
		{
			x_end = x_start /* - y_start */;
			x32 = xw[x_start];
			if (x32 < y32 && x_end > 0)
			{
				x_end--;
				x32 = (x32 << 16) + xw[x_end];
			}

			res = x32 / y32;
			result.w[x_end] += res;

			/* This can be optimized for one or two words.
			 */
			borrow = 0;
			res *= y32;
			for (j = x_end; j <= x_start; j++)
			{
				xx = (os_ushort)res;
				next_borrow = (os_ushort)((os_uint)xx
					+ (os_uint)borrow > (os_uint)xw[j]);
				xw[j] -= xx + borrow;
				borrow = next_borrow;
				res >>= 16;
			}

			do 
			{
				if (x_start-- == 0) goto return_result;
			} 
			while (xw[x_start] == 0);
		}
	}

	/* Divisor has two or more words.
	 */
	y32_original = (((os_uint)yw[y_start]) << 16) + (os_uint)yw[y_start-1];
	while (1)
	{
		y32 = y32_original;
		x_end = x_start - y_start;
		x32 = (((os_uint)xw[x_start]) << 16) + (os_uint)xw[x_start-1];
		if (x32 <= y32)
		{
			if (x32 == y32) 
			{
				j = (os_short)y_start - 1; 
				do
				{
					if (--j < 0) goto goon;
					xx = xw[j+x_end];
					yy = yw[j];
					if (xx > yy) goto goon;
				} 
				while (xx == yy);
			}

			if (x_end == 0) goto return_result;

			x_end--;
			y32 >>= 16;
		}
goon:
		res = x32 / (y32+1);
		if (res == 0) res = 1;
		result.w[x_end] += res;

		osal_int64_set_zero(&tmp);
		tmp.w[x_end] = res; 

		osal_int64_unsigned_multiply(y, &tmp, &tmp);
		osal_int64_subtract(x, &tmp);

		while (xw[x_start] == 0)
		{
			if (x_start-- < y_start+1) goto return_result;
		} 
	}

return_result:
	osal_int64_copy(x, &result);
	return;

return_zero:
	osal_int64_set_zero(x);
}


/* End OSAL_INT64X_SUPPORT and OSAL_LONG_IS_64_BITS == 0
 */
#endif
#endif
