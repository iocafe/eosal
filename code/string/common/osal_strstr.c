/**

  @file    string/commmon/osal_strchr.c
  @brief   Find a substring within a string.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"


/**
****************************************************************************************************

  @brief Find a substring within a string.
  @anchor os_strstr

  The os_strstr() function finds the first occurrence of substr within string, 
  or it returns OS_NULL if the substring is not found. 

  @param   str Pointer to string from which to search. If the str is OS_NULL the function
		   will return OS_NULL.
  @param   substr Substring to search for. If the substr is OS_NULL the function
		   will return OS_NULL. If substr is pointer to null character, the function
		   returns pointer to str.
  @param   flags Set OSAL_STRING_DEFAULT for normal operation, much like strstr(). 
		   Flag OSAL_STRING_SEARCH_LINE_ONLY causes search to stop on first new line
		   character or semicolon.
		   Flag OSAL_STRING_SEARCH_ITEM_NAME is used to find a named item from list
		   string. See function osal_str_get_item_value() for more information.

  @return  Pointer to the first occurrence of substring within the string, or OS_NULL if 
		   substr is not found.

****************************************************************************************************
*/
os_char *os_strstr(
    const os_char *str,
	const os_char *substr,
	os_short flags)
{
	os_char c, d, e, *s, *p, *q;
	const os_char *u;
	os_boolean quoted;

	/* If there is nothing to do, just return OS_NULL.
	 */
	if (str == OS_NULL || substr == OS_NULL) return OS_NULL;

	/* First character of substring is
	 */
	c = *substr;
	if (c == '\0') return (os_char*)str;

	p = (os_char*)str;
	quoted = OS_FALSE;
	do 
	{
		e = *p;

		if (e == '\"') quoted = !quoted;

		/* If first character matches.
		 */
		if (c == e)
		{
			/* If we need to find item by name 
			 */
			if (flags & OSAL_STRING_SEARCH_ITEM_NAME)
			{
				if (quoted) goto skipthis;
				q = p;
				while (q-- != str)
				{
					d = *q;
					if (d == '\n' || d == ';' || d == '\t' || d == ',') break;
					if (!osal_char_isspace(d)) goto skipthis;
				}
			}

			/* Check the rest of substring characters.
			 */
			s = p;
			u = substr;
			do 
			{
				/* If whole substring has been compared.
				 */
				if (*(++u) == '\0') 
				{
					/* If searching for item name, make sure it ends here.
					 */
					if (flags & OSAL_STRING_SEARCH_ITEM_NAME)
					{
						d = s[1];
						if (osal_char_isaplha(d) || osal_char_isdigit(d)) 
							goto skipthis;
					}
					return (os_char*)p;
				}
			} 
			while (*u == *(++s));
		}
skipthis:

		/* If we need to limit the search to the first line. Lines are separated
		   wither by new line character or semicolon.
		 */
        if (flags & OSAL_STRING_SEARCH_LINE_ONLY)
		{
			if ((e == '\n' || e == ';') && !quoted) break;
		}
	} 
	while (*(p++));

	/* Not found.
	 */
	return OS_NULL;
}
