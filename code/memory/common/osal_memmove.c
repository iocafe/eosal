/**

  @file    memory/commmon/osal_memmove.c
  @brief   Move memory.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"

#ifndef OSAL_MEMORY_OS_CLR_AND_CPY

/**
****************************************************************************************************

  @brief Move a memory range.
  @anchor os_memmove

  The os_memmove() function moves count bytes of src to dst. If the source and destination
  overlap, this function ensures that the original source bytes in the overlapping region
  are copied before being overwritten. 

  Most often this function is not used, but C standard library function. Standard library function
  is much faster than this simple implementation. Search with OSAL_MEMORY_OS_CLR_AND_CPY string.

  @param   dst Destination pointer.
  @param   src Source pointer.
  @param   count Number of bytes to copy If bytes zero or negative, the function does nothing. 

  @return  None.

****************************************************************************************************
*/
void os_memmove(
    void *dst,
    const void *src,
    os_memsz count)
{
	register os_char *d;
    register const os_char *s;

    if (dst == OS_NULL || src == OS_NULL || dst == src)
	{
		return;
	}

	d = dst;
	s = src;

    if (d < s)
    {
        while (count-- > 0)
        {
            *(d++) = *(s++);
        }
    }
    else
    {
        d += count;
        s += count;

        while (count-- > 0)
        {
            *(--d) = *(--s);
        }
    }
}

#endif
