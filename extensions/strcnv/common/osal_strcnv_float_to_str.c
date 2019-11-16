/**

  @file    strcnv/common/osal_double_to_str.c
  @brief   Convert floating point number to string.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Function osal_double_to_str() to convert floating point number to string.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_STRCONV_SUPPORT


/**
****************************************************************************************************

  @brief Convert floating point number to string.
  @anchor osal_double_to_str

  The osal_double_to_str() function converts a double precision floating point value
  to string. 

  @param   buf Pointer to buffer to store string into.
  @param   buf_sz Buffer size in bytes, at least 30 characters.
  @param   x Floating point value to convert.
  @param   ddigs Number of decimal digits after decimal point.
  @param   flags OSAL_FLOAT_DEFAULT (0) for normal format, or OSAL_FLOAT_E_FORMAT (1) for the
		   E format.

  @return  Number of bytes needed to store the resulting string, including terminating null 
		   character. Value 1 indicates an error.

****************************************************************************************************
*/
os_memsz osal_double_to_str(
    os_char *buf, 
    os_memsz buf_sz,
    os_double x, 
    os_int ddigs,
	os_int flags)
{
    os_int c, j, m, ndig;
    os_double y;
    os_char *s, *e;

    /* Set s to point position where to store the next character, 
	   and e to point end of buffer.
     */
    s = buf;
    e = buf + buf_sz - 1;

	ndig = (ddigs<0) ? 7 : (ddigs > 22 ? 23 : ddigs+1);
	c = 0;

    /* Start negative values with minus sign. 
	   Handle as positive for the rest of code.
     */
	if (x < 0)
	{
		x = -x;
		*s++ = '-';
	}

	/* Scale the value to range 1 <= value < 10.
	 */
	if (x > 0.0) while (x < 1.0)
	{
		x *= 10.0;
		c--;
	}
	while (x >= 10.0)
	{
		x *= 0.1; // x = x/10.0;
		c++;
	}

	/* In normal format (not E format), number of digits depends on size 
	   of the value.
	 */
	if ((flags & OSAL_FLOAT_E_FORMAT) == 0) 
	{
		ndig += c;
	}

	/* Round. the value is between 1 and 10 and ndig will be printed to
	   right of decimal point so rounding is:
	 */
	y = 1.0;
	for (j = 1; j < ndig; j++)
	{
		y /= 10.0;
	}
	x += y / 2.0;

	/* Correct if rounded to over 10.
	 */
	if (x >= 10.0) {x = 1.0; c++;} 

	/* Normal format (not the E format): Write leading zeroes.
	 */
	if ((flags & OSAL_FLOAT_E_FORMAT) == 0 && c<0)
	{
		if (s + (1-c) > e) goto getout;

		*s++ = '0'; 
		*s++ = '.';
		if (ndig < 0) c = c - ndig;
		for (j = -1; j > c; j--)
			*s++ = '0';
	}

	/* Write the significant digits.
	 */
	if (s+ndig > e) goto getout;
	for (j=0; j < ndig; j++)
	{
		m = (os_int)x;
		*s++ = m + '0';

		/* If this is place for the decimal point.
		 */
		if (j == ((flags & OSAL_FLOAT_E_FORMAT) == 0 ? c : 0))
		{
			if ((flags & OSAL_FLOAT_E_FORMAT) || j != ndig-1) 
			{
				*s++ = '.';
			}
		}

		x -= (y=m);
		x *= 10.0;
	}

	/* E mode, write the exponent.
	 */
	if ((flags & OSAL_FLOAT_E_FORMAT) && c != 0)
	{
		if (s >= e) goto getout;
		*s++ = 'E';
		if (c < 0)
		{
			c = -c;
			if (s >= e) goto getout;
			*s++ = '-';
		}

		for (m = 100; m > c; m /= 10);

		while (m > 0)
		{
			if (s >= e) goto getout;
			*s++ = c/m + '0';
			c = c % m;
			m /= 10;
		}
	}
	*(s++) = '\0';

	return s - buf;

	/* Failed, buffer overun.
	 */
getout:
	buf[0] = '\0';
	return 1;
}

#endif
