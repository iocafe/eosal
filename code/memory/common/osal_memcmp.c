/**

  @file    memory/commmon/osal_memcmp.c
  @brief   Copy memory.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"

#ifndef OSAL_MEMORY_OS_CLR_AND_CPY

/**
****************************************************************************************************

  @brief Compare memory range.
  @anchor os_memcpy

  The os_memcpy() function copies count bytes of str2 to str1. If the source and destination
  overlap, this function does not ensure that the original source bytes in the overlapping region 
  are copied before being overwritten. 

  Most often this function is not used, but C standard library function. Standard library function
  is much faster than this simple implementation. Search with OSAL_MEMORY_OS_CLR_AND_CPY string.

  @param   str1 Destination pointer.
  @param   str2 Source pointer.
  @param   count Number of bytes to copy If bytes zero or negative, the function does nothing. 

  @return The function returns -1 if str1 is less than str2, 0 if strings are equal,
          or 1 is str1 is greater than str2.

****************************************************************************************************
*/
os_int os_memcmp(
    const void *str1,
    const void *str2,
    os_memsz count)
{
    const register os_uchar *s1 = str1, *s2 = str2;

    if (s1 == OS_NULL || s2 == OS_NULL)
	{
        if (s1 == s2) return 0;
        return s1 == OS_NULL ? -1 : 1;
	}

	while (count-- > 0)
	{
        if (*s1 != *s2)
        {
            return s1 < s2 ? -1 : 1;
        }

        s1++;
        s2++;
	}
    return 0;
}

#endif
