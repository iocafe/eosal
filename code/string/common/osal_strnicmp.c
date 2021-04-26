/**

  @file    string/common/osal_strnicmp.c
  @brief   Compare two strings.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"


/**
****************************************************************************************************

  @brief Compare two strings, ignore case, limit maximum string length.
  @anchor os_strnicmp

  The os_strnicmp() function compares two strings, the comparation is case insensitive.
  Maximum count characters are compared. NULL pointers are treated as empty strings.

  @param  str1 Pointer to the first string.
  @param  str2 Pointer to the second string.
  @param  count Maximum number of characters to compare. To compare string without this limit
          set count parameter to -1.

  @return The function returns -1 if str1 is less than str2, 0 if strings are equal,
          or 1 is str1 is greater than str2.

****************************************************************************************************
*/
os_int os_strnicmp(
    const os_char *str1,
    const os_char *str2,
    os_long count)
{
    os_char c1, c2;

    /* NULL string is same as empty string.
     */
    if (str1 == OS_NULL) str1 = osal_str_empty;
    if (str2 == OS_NULL) str2 = osal_str_empty;

    /* Compare strings. Return 0 if string match.
     */
    while (OS_TRUE)
    {
        c1 = *(str1++);
        c2 = *(str2++);
        c1 = osal_char_tolower(c1);
        c2 = osal_char_tolower(c2);

        if (count-- == 0) return 0;
        if (c1 != c2) break;
        if (c1 == '\0') return 0;
    }

    return (os_int)((c1 < c2) ? -1 : 1);
}

