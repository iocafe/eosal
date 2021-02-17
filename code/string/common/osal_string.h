/**

  @file    string/common/osal_string.h
  @brief   String manipulation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  This header file contains functions prototypes for string manipulation.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_STRING_H_
#define OSAL_STRING_H_
#include "eosal.h"

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
    osal_str_get_item_value(), osal_str_get_item_int()...  functions will search only
    first line of the sting.
 */
#define OSAL_STRING_SEARCH_LINE_ONLY 2

/** Recommended number conversion buffer for osal_int_to_str(), etc.
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
osalStatus os_strncat(
    os_char *dst,
    const os_char *src,
    os_memsz dst_size);

/* Compare two strings, case sensitive.
 */
os_int os_strcmp(
    const os_char *str1,
    const os_char *str2);

/* Compare n first characters of two strings.
 */
os_int os_strncmp(
    const os_char *str1,
    const os_char *str2,
    os_memsz n);

/* Compare two strings, ignore case, limit maximum string length.
 */
os_int os_strnicmp(
    const os_char *str1,
    const os_char *str2,
    os_long count);

/* Get string size in bytes (including terminating '\0' character).
 */
os_memsz os_strlen(
    const os_char *str);

/* Find a character within string.
 */
os_char *os_strchr(
    const os_char *str,
    os_uint c32);

/* Find last matching character within string.
 */
os_char *os_strechr(
    const os_char *str,
    os_uint c32);

/* Find a substring within a string. Used also to find named items from list.
 */
os_char *os_strstr(
    const os_char *str,
    const os_char *substr,
    os_short flags);

/* Convert integer to string.
 */
os_memsz osal_int_to_str(
    os_char *buf,
    os_memsz buf_sz,
    os_long x);

/* Convert string to integer.
 */
os_long osal_str_to_int(
    const os_char *str,
    os_memsz *count);

/*@}*/

#endif
