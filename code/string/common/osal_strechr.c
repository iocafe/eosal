/**

  @file    string/common/osal_strechr.c
  @brief   Find last matching character in string.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"


/**
****************************************************************************************************

  @brief Find last matching character within string.
  @anchor os_strechr

  The os_sterchr() function finds the last occurrence of c in string, or it 
  returns OS_NULL if c is not found. The terminating null character is included in the 
  search.

  @param   str Pointer to string from which to search. If the str is OS_NULL the function
		   will return OS_NULL.
  @param   c32 UTF32 character to search for. Current implementation works only for 
           ASCII characters from 0 ... 0x7F.

  @return  Pointer to the last occurrence of c in string, or OS_NULL if c is not found.

****************************************************************************************************
*/
os_char *os_strechr(
    os_char *str,
	os_uint c32)
{
	os_char c, *p;

#if OSAL_UTF8
	os_char substr[8];
	os_int n;
#endif

	/* If string pointer is OS_NULL, nothing can be found.
	 */
	if (str == OS_NULL) return OS_NULL;

    /* No match found yet
     */
    p = OS_NULL;

#if OSAL_UTF8
	/* Characters above 127 will do require more than one UTF8 byte, convert UTF32 character
	   to UTF8 string, and search with string.
	 */
	if (c32 >= 0x80) 
	{
		/* Convert character to UTF8 string.
		 */
		n = osal_char_utf32_to_utf8(substr, sizeof(substr), c32);

		/* If illegal character, return OS_NULL to indicate that character was not found.
		 */
		if (n <= 0) return OS_NULL;

		/* Terminate the string with null character.
		 */
		substr[n] = '\0';

		/* Use find substring function.
		 */
        while (OS_TRUE)
        {
		    str = os_strstr(str, substr, OSAL_STRING_DEFAULT);
            if (str == OS_NULL) return p;
            p = str;
            str += n;
        }
	}
#endif

	/* Find ASCII character.
	 */
	c = (os_char)c32;
	do 
	{
		if (*str == c) p = str;
	} 
	while (*(str++));

	/* Return pointer to last match, or OS_NULL is none.
	 */
	return p;
}
