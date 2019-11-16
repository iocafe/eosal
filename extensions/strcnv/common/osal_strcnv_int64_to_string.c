/**

  @file    strcnv/common/osal_int64_to_string.c
  @brief   Convert 64 bit integer to string.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Function osal_int64_to_string() converts 64 bit integer number to string.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_STRCONV_SUPPORT

/* This code is needed only if os_long is not 64 bits.
 */
#if OSAL_LONG_IS_64_BITS == 0

/**
****************************************************************************************************

  @brief Convert 64 bit integer to string.
  @anchor osal_int64_to_string

  The osal_int64_to_string() function converts 64 bit integer to string. 
  If the resulting string does not fit into buffer, buffer will contain "?" string.

  @param   buf Pointer to buffer to store string into.
  @param   buf_sz Buffer size in bytes, at least 21 characters.
  @param   x Pointer to 64 bit integer value to convert.

  @return  Number of bytes needed to store the resulting string, including terminating null 
		   character. Value 1 indicates an error.

****************************************************************************************************
*/
os_memsz osal_int64_to_string(
    os_char *buf, 
    os_memsz buf_sz,
    os_int64 *x)
{
    os_uint low, high;
    os_int64 i1, i2, i3;
    os_char tmpbuf[22], *b, *dst;
    os_boolean negative;

	/* Check function arguments.
	 */
	if (buf == OS_NULL || buf_sz < 21) 
	{
		osal_debug_error("Buffer not acceptable");
		if (buf && buf_sz > 0) *buf = '\0';
		return 1;
	}

    dst = buf;

	/* If the value is zero, just output zero.
	 */
    if (osal_int64_is_zero(x)) 
    {
        *(dst++) = '0'; 
        goto getout;
    }

	/* Set initil values.
	 */
    osal_int64_copy(&i1, x);
    b = tmpbuf + sizeof(tmpbuf) - 1;

	/* If value is negative, set negative flag and treat as positive value.
	 */
    negative = osal_int64_is_negative(&i1);
    if (negative) 
    {
        osal_int64_negate(&i1);
    }

    do 
    {
	    osal_int64_copy(&i2, &i1);
		osal_int64_divide(&i1, &osal_int64_10);
	    osal_int64_copy(&i3, &i1);
		osal_int64_multiply(&i3, &osal_int64_10);
		osal_int64_subtract(&i2, &i3);
	
		osal_int64_get_uint2(&i2, &low, &high);
        *(b--) = (os_char)('0' + low);
    }
    while (!osal_int64_is_zero(&i1));

	/* If negative write minus sign.
	 */
    if (negative) *(dst++) = '-';

	/* Write the decimal digits.
	 */
    while (++b < tmpbuf + sizeof(tmpbuf)) 
    {
        *(dst++) = *b;
    }

getout:
    *(dst++) = '\0'; 
    return dst - buf;
}

#endif
#endif
