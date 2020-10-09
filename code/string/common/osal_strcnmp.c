/**

  @file    string/common/osal_strncmp.c
  @brief   Compare two strings.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    1.10.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"


/**
****************************************************************************************************

  @brief Compare n first characters of two strings.
  @anchor os_strncmp

  The os_strncmp() function compares first n characters of two strings, the comparation is case
  sensitive. NULL pointers are treated as empty strings.

  @param  str1 Pointer to the first string.
  @param  str2 Pointer to the second string.
  @param  n Number of characters to compare.

  @return The function returns -1 if str1 is less than str2, 0 if strings are equal,
          or 1 is str1 is greater than str2.

****************************************************************************************************
*/
os_int os_strncmp(
    const os_char *str1,
    const os_char *str2,
    os_memsz n)
{
    /* NULL string is same as empty string.
     */
    if (str1 == OS_NULL) str1 = osal_str_empty;
    if (str2 == OS_NULL) str2 = osal_str_empty;

    /* Compare strings. Return 0 if string match.
     */
    while (*str1 == *str2)
    {
        if (--n <= 0) return 0;
        if (*str1 == '\0') return 0;
        str1++;
        str2++;
    }

    return (os_int)((*str1 < *str2) ? -1 : 1);
}

