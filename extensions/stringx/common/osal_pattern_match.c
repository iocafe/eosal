/**

  @file    stringx/common/osal_pattern_match.c
  @brief   Wild card pattern matching.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

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

  @brief Check if string matches to pattern.
  @anchor osal_pattern_match

  The osal_pattern_match() function cheks is string matches to given pattern. This is typically
  used to get specific file names from directory list, etc.

  @param   str String, like path or file name.
  @param   pattern Pattern to look for, like "*.txt".
  @param   flags Reserved for future, set 0 for now.

  @return  OS_TRUE if pattern matches to string, OS_FALSE if not.

****************************************************************************************************
*/
os_boolean osal_pattern_match(
    const os_char *str,
    const os_char *pattern,
    os_int flags)
{
    /* If we reach at the end of both strings, we are done.
     */
    if (*pattern == '\0' && *str == '\0')
        return OS_TRUE;

    /* Make sure that the characters after '*' are present
       in str string. This function assumes that the pattern
       string will not contain two consecutive '*'
     */
    if (*pattern == '*' && *(pattern+1) != '\0' && *str == '\0')
        return OS_FALSE;

    /* If the pattern string contains '?', or current characters
       of both strings match
     */
    if (*pattern == '?' || *pattern == *str)
        return osal_pattern_match(str + 1, pattern + 1, flags);

    /* If there is *, then there are two possibilities
       a) We consider current character of str string
       b) We ignore current character of str string.
     */
    if (*pattern == '*')
        return osal_pattern_match(str, pattern + 1, 0) || osal_pattern_match(str + 1, pattern, 0);

    return OS_FALSE;
}

#endif
