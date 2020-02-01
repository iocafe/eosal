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

/* Flags for osal_double_to_str() function.
 */
#define OSAL_FLOAT_DEFAULT 0
#define OSAL_FLOAT_E_FORMAT 1


/** 
****************************************************************************************************

  @name Extended string manipulation and conversions functions

  These include treating strings as lists, converting strings to integers, IP/MAC address
  manipulation and primitive patter matching.

****************************************************************************************************
 */
/*@{*/

/* Convert floating point number to string.
 */
os_memsz osal_double_to_str(
    os_char *buf,
    os_memsz buf_sz,
    os_double x,
    os_int ddigs,
    os_int flags);

/* Convert string to floating point number.
 */
os_double osal_str_to_double(
    const os_char *str,
    os_memsz *count);

/* #define osal_int64_to_str(b,s,x) osal_int_to_str((b),(s),*(x)) */

/* Convert a string to 64 bit integer.
 */
os_memsz osal_str_to_int64(
    os_int64 *x,
    os_char *str);

/* Convert 64 bit integer to string. If operating system supports 64 bit integers as
   os_long type, we just use macro to map this to osal_int_to_str() function.
   If operating system has no 64 bit support, the function implementation is used.
 */
#if OSAL_LONG_IS_64_BITS
    #define osal_int64_to_str(b,s,x) osal_int_to_str((b),(s),*(x))
#else
    os_memsz osal_int64_to_str(
        os_char *buf,
        os_memsz buf_sz,
        os_int64 *x);
#endif

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
os_int osal_str_to_bin(
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

/* Convert binary IP address to string.
 */
void osal_ip_to_str(
    os_char *str,
    os_memsz str_sz,
    const os_uchar *ip,
    os_memsz ip_sz);

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
