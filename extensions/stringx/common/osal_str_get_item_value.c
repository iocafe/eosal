/**

  @file    stringx/common/osal_str_get_item_value.c
  @brief   Find value of item in list string.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

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

  @brief Find a value of specified list item.
  @anchor osal_str_get_item_value

  The osal_str_get_item_value() function finds the value of first of item with given
  item name within a list string. This function is related to processing of list strings,
  which are used to pass parameters, etc. List string consists of items, arranged as table
  with columns and rows columns. Each column is separated by comma or tabulator character
  and each row by semicolon or new line character. Items themselfs consist of item name
  and item value. These may be separated by '=' character or space. Item value may be
  quoted, but it doesn't have to be. Advantage of quoted values are that these can contain
  any characters, including comma, semicolon, tabulator or new line. 
  
  For example list could be 'ip="192.168.1.232:22",timeout=2500'. Calling
  osal_str_get_item_value() on this string with item name "ip" would return
  pointer to '192.168.1.232:22",timeout=2500' position within this string
  and set n_chars to 16.

  @param   list_str Pointer to list string to search. If the str is OS_NULL the function
		   will return OS_NULL.
  @param   item_name Pointer to list item name to search for. If this is OS_NULL,
		   the function will return OS_NULL. 
  @param   n_chars Pointer to integer into which to store the number of characters in
		   item value. This can be OS_NULL if not needed.
  @param   flags Set OSAL_STRING_DEFAULT to search whole list string, or 
		   OSAL_STRING_SEARCH_LINE_ONLY to search only from first line.

  @return  If item with name was found, pointer to first character of actual item value. 
		   If item has no value, the function returns pointer to empty string.
		   If item with the specified name was not found at all, the function returns
		   OS_NULL.

****************************************************************************************************
*/
const os_char *osal_str_get_item_value(
    const os_char *list_str,
	const os_char *item_name,
	os_memsz *n_chars,
	os_short flags)
{
    os_char c;
    const os_char *p, *start;

	/* Find item by name.
	 */
	p = os_strstr(list_str, item_name, 
		(os_short)(flags|OSAL_STRING_SEARCH_ITEM_NAME));

	/* If not found, just return OS_NULL.
	 */
	if (p == OS_NULL) return OS_NULL;

	/* Move on by item name's length.
	 */
	p += os_strlen(item_name) - 1;

	/* Skip space and '=' character.
	 */
	while (1)
	{
		c = *p;
		if (c == '\0' || c == '\t' || c == ',' || c == '\n' || c == ';') goto getout;
		if (!osal_char_isspace(c) && c != '=') break;
		p++;
	}

	/* If this is quoted value. Look for end quotation mark.
	 */
	if (c == '\"')
	{
		start = p+1;
		while (*(++p) != '\"') if (*p == '\0') goto getout;
	}

	/* Not quoted value
	 */
	else
	{
		start = p;
		while (1)
		{
			c = *p;
			if (c == '\0' || c == '\t' || c == ',' || c == '\n' || c == ';') break;
			p++;
		}

		while (p-- != start)
		{
			if (!osal_char_isspace(*p)) break;
		}
		p++;
	}

	if (n_chars) *n_chars = (os_memsz)(p - start);
	return start;

getout:
	if (n_chars) *n_chars = 0;
	return "";
}


/**
****************************************************************************************************

  @brief Find beginning of next line in list string.
  @anchor osal_str_get_next_line

  The osal_str_get_next_line() function returns pointer to beginning of next line 
  in list string
  

  @param   list_str Pointer to list string to search. If the str is OS_NULL the function
		   will return pointer to null character.

  @return  Pointer to first character on next line of list string.

****************************************************************************************************
*/
const os_char *osal_str_get_next_line(
    const os_char *list_str)
{
    const os_char *a, *b;

	/* If list string is NULL pointer, return pointer to null character.
	 */
	if (list_str) 
	{
        a = os_strchr((os_char*)list_str, ';');
        b = os_strchr((os_char*)list_str, '\n');
		if (a) 
		{
			if (b) if (b < a) a = b;
			return a+1;
		}
		
		if (b) return b+1;
	}

	/* No next line.
	 */
	return "";
}

#endif
