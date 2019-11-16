/**

  @file    string/common/osal_int_to_str.c
  @brief   Convert integer to string.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"


/**
****************************************************************************************************

  @brief Convert integer to string.
  @anchor osal_int_to_str

  The osal_int_to_str() function converts an integer x to string and stores the resulting
  string into buffer. If the buffer is too small to hold the resulting string, the function
  stores null character to buffer and returns 1.

  @param   buf Pointer to buffer into which to store the resulting string. 
  @param   buf_sz Buffer size in bytes. The resulting string should fit into buffer, minimum 
		   buffer size is 21 characters. Common number conversion buffer size is OSAL_NBUF_SZ.
  @param   x Integer value to convert.

  @return  Number of bytes needed to store the resulting string, including terminating null 
		   character. Value 1 indicates an error.

****************************************************************************************************
*/
os_memsz osal_int_to_str(
    os_char *buf,
	os_memsz buf_sz,
	os_long x)
{
    os_char *p, *t, tmp[22];

	/* Check function arguments.
	 */
	if (buf == OS_NULL || buf_sz < 21) 
	{
		osal_debug_error("Buffer not acceptable");
		if (buf && buf_sz > 0) *buf = '\0';
		return 1;
	}

	/* If negative number, place minus sign to buffer and make x positive.
	 */
	p = buf;
	if (x < 0)
	{
		x = -x;
		*(p++) = '-';
	}

	/* Convert the unsigned integer to decimal digits. This will fill in tmp
	   buffer, digits in reverse order.
	 */
	t = tmp;
	while (x)
	{
		*(t++) = (os_char)(x % 10) + '0';
		x /= 10;
	}

	/* If we got the number, reverse character and store to buf.
	 */
	if (t != tmp)
	{
		while (t-- != tmp) *(p++) = *t;
	}

	/* Handle zero.
	 */
	else
	{
		*(p++) = '0';
	}

	/* Null terminate.
	 */
	*(p++) = '\0';

	/* Return number of characters including null character.
	 */
	return p - buf;
}
