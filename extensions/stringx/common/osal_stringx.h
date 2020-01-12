/**

  @file    stringx/common/osal_stringx.h
  @brief   String manipulation, extended functions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  This header file contains functions prototypes for more string manipulation.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#if OSAL_STRINGX_SUPPORT

/* Flags for osal_str_get_next() function.
 */
#define OSAL_STR_NEXT_ITEM 0
#define OSAL_STR_NEXT_LINE 1


/** 
****************************************************************************************************

  @name Extended string manipulation and conversions functions

  These include treating strings as lists, converting strings to integers, IP/MAC address
  manipulation and primitive patter matching.

****************************************************************************************************
 */
/*@{*/

/* Find a value of specified list item.
 */
const os_char *osal_str_get_item_value(
    const os_char *list_str,
	const os_char *item_name,
	os_memsz *n_chars,
	os_short flags);

/* Get integer value of specified list item.
 */
os_long osal_str_get_item_int(
    const os_char *list_str,
    const os_char *item_name,
    os_long default_value,
    os_short flags);

/* Find beginning of next line in list string.
 */
const os_char *osal_str_get_next(
    const os_char *list_str,
    os_short flags);

/* Iterate trough items in string list.
 */
osalStatus osal_str_list_iter(
    os_char *buf,
    os_memsz buf_sz,
    const os_char **list_str_ptr,
    os_short flags);

/* Convert string to integer.
 */
os_long osal_str_to_int(
    const os_char *str,
	os_memsz *count);

/* Convert hexadecimal string to integer.
 */
os_long osal_hex_str_to_int(
    const os_char *str,
    os_memsz *count);

/* Convert string to list of numbers.
 */
os_int osal_str_to_list(
    os_ushort *x,
    os_short n,
    const os_char *str,
    os_char c,
    os_short b);

/* Convert string to binary IP address.
 */
osalStatus osal_ip_from_str(
    os_uchar *ip,
    os_memsz ip_sz,
    const os_char *str);

/* Convert string to binary MAC address.
 */
osalStatus osal_mac_from_str(
    os_uchar mac[6],
    const char* str);

/* Check if string matches to pattern.
 */
os_boolean osal_pattern_match(
    const os_char *str,
    const os_char *pattern,
    os_int flags);

/*@}*/

#endif
