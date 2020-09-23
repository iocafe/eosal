/**

  @file    stringx/common/osal_str_ends_with.c
  @brief   Check is a string ends with a substring.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    23.9.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_STRINGX_SUPPORT

/**
****************************************************************************************************

  @brief Check if string ends with "ending".
  @anchor osal_str_ends_with

  The osal_str_ends_with() function cheks is the string "str" given as argument. If so, the
  function returns pointer to ending within the string. This function is often used to
  check if path has a trailing slash.

  @param   str String to check.
  @param   ending Ending, like "/".

  @return  If "str" ends with "ending", the function returns pointer to ending within str.
           If not, the function returns OS_NULL

****************************************************************************************************
*/
const os_char *osal_str_ends_with(
    const os_char *str,
    const os_char *ending)
{
    os_memsz len, lastpos;

    len = os_strlen(ending);
    lastpos = os_strlen(str) - len;
    if (lastpos >= 0) {
        if (!os_strcmp(str + lastpos, ending)) {
            return str + lastpos;
        }
    }
    return OS_NULL;
}

#endif
