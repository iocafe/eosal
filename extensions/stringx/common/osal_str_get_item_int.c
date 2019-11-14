/**

  @file    stringx/common/osal_str_get_item_int.c
  @brief   Get integer value of an item in list string.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_STRINGX_SUPPORT


/**
****************************************************************************************************

  @brief Get integer value of specified list item.
  @anchor osal_str_get_item_int

  The osal_str_get_item_int() function finds the value of first of item with given
  item name within a list string, and returns item's value as integer. 
  See function osal_str_get_item_value() for more information about list strings.

  @param   list_str Pointer to list string to search. If the str is OS_NULL the function
		   will return the default value.
  @param   item_name Pointer to list item name to search for. If this is OS_NULL,
		   the function will return the default value. 
  @param   default_value Returned if the item with given name is not found or has no value.
  @param   flags Set OSAL_STRING_DEFAULT to search whole list string, or 
		   OSAL_STRING_SEARCH_LINE_ONLY to search only from first line.

  @return  If item with name was found and has value, the function returns the value
		   as integer. Otherwise the function will return default value given as
		   argument.

****************************************************************************************************
*/
os_long osal_str_get_item_int(
    const os_char *list_str,
	const os_char *item_name,
	os_long default_value,
	os_short flags)
{
    const os_char *str_value;
    os_memsz count;
    os_long x;

	/* Get pointer to value as string.
	 */
	str_value = osal_str_get_item_value(list_str, item_name, OS_NULL, flags);

	/* If we got string value convert it to integer. Use value of conversion to
	   integer succeeded.
	 */
	if (str_value) 
	{
		x = osal_str_to_int(str_value, &count);
		if (count) default_value = x;
	}

	/* Return the value.
	 */
	return default_value;
}

#endif
