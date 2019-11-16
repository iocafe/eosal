/**

  @file    stringx/common/osal_hex_str_to_int.c
  @brief   Convert hexadecimal string to integer.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.12.2017

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_STRINGX_SUPPORT


/**
****************************************************************************************************

  @brief Convert hexadecimal string to integer.
  @anchor osal_hex_str_to_int

  The osal_hex_str_to_int() function converts a string to integer. If no integer is found, the
  function sets count to zero and returns zero.

  @param   str Pointer to string to convert to integer. 
  @param   count Pointer to integer into which to store number of parsed characters. The parsed
           characters include white space and minus sign. Set to zero if function failed.
           This can be OS_NULL if not needed.

  @return  Integer value parsed from the string.

****************************************************************************************************
*/
os_long osal_hex_str_to_int(
    const os_char *str,
	os_memsz *count)
{
    os_char c, v;
    const os_char *p, *q;
    os_long x;
    os_boolean negative;

    /* Check that str is not NULL pointer.
	 */
	if (str == OS_NULL) goto getout;

	/* Skip the white space at beginning.
	 */
	p = str;
	while (1)
	{
		c = *p;
		if (c == '\0') goto getout;
		if (!osal_char_isspace(c)) break;
		p++;
	}
	
	/* Check for minus sign
	 */
	negative = OS_FALSE;
	if (c == '-' || c == '+')
	{
		if (c == '-') negative = OS_TRUE;
		p++;

		/* Skip the white space between minus sign and first digit.
		 */
		while (1)
		{
			c = *p;
			if (c == '\0') goto getout;
			if (!osal_char_isspace(c)) break;
			p++;
		}
	}

	/* Parse the number
	 */
	x = 0;
    q = p;
	while (OS_TRUE)
	{
		c = *p;

        if (osal_char_isdigit(c))
        {
            v = c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            v = c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F')
        {
            v = c - 'A' + 10;
        }
        else
        {
            break;
        }

        x = x * 16 + (os_long)v;
        p++;
    }

    /* Number must contain at least one decimal digit.
     */
    if (q == p) goto getout;

	/* If negative, negate.
	 */
	if (negative) x = -x;

	/* Set number of parsed bytes, if needed.
	 */
	if (count) *count = p - str;

	/* Return the integer value
	 */
	return x;

getout:
	/* Failed, set count to sero and return zero.
	 */
	if (count) *count = 0;
	return 0;
}

#endif
