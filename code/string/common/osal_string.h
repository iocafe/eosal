/**

  @file    string/common/osal_string.h
  @brief   String manipulation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This header file contains functions prototypes for string manipulation.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_STRING_INCLUDED
#define OSAL_STRING_INCLUDED


/** 
****************************************************************************************************

  @name Flags For String Functions

  The ...

****************************************************************************************************
 */
/*@{*/

/** Default flag. Use the OSAL_STRING_DEFAULT (0) to specify that there are no specifal flags.
 */
#define OSAL_STRING_DEFAULT 0

/** Search for item name only. If specified the os_strchr() function will 
    search only for item name, whole word, not value.
 */
#define OSAL_STRING_SEARCH_ITEM_NAME 1 

/** Search first line only. If specified the os_strchr(), 
    osal_string_get_item_value(), osal_string_get_item_int()...  functions will search only
	first line of the sting.
 */
#define OSAL_STRING_SEARCH_LINE_ONLY 2

/** Recommended number conversion buffer for osal_int_to_string(), etc.
 */
#define OSAL_NBUF_SZ 32


/*@}*/

/** 
****************************************************************************************************

  @name String Manipulation Functions

  The ...

****************************************************************************************************
 */
/*@{*/

/* Copy string.
 */
void os_strncpy(
    os_char *dst,
    const os_char *src,
    os_memsz dst_size);

/* Append a string to another string.
 */
void os_strncat(
    os_char *dst,
    const os_char *src,
    os_memsz dst_size);

/* Compare two strings, case sensitive.
 */
os_int os_strcmp(
    const os_char *str1,
    const os_char *str2);

/* Compare two strings, ignore case, limit maximum string length.
 */
os_int os_strnicmp(
    const os_char *str1,
    const os_char *str2,
    os_long count);

/* Get string size in bytes.
 */
os_memsz os_strlen(
    const os_char *str);

/* Find a character within string.
 */
os_char *os_strchr(
    os_char *str,
	os_uint c32);

/* Find last matching character within string.
 */
os_char *os_strechr(
    os_char *str,
	os_uint c32);

/* Find a substring within a string. Used also to find named items from list.
 */
os_char *os_strstr(
    const os_char *str,
	const os_char *substr,
	os_short flags);

/* Convert integer to string.
 */
os_memsz osal_int_to_string(
    os_char *buf,
	os_memsz buf_sz,
	os_long x);

/*@}*/

#endif
