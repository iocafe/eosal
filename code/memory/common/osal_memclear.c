/**

  @file    memory/commmon/osal_memclear.c
  @brief   Clearing memory.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"

#ifndef OSAL_MEMORY_OS_CLR_AND_CPY

/**
****************************************************************************************************

  @brief Set memory range to zeroes.
  @anchor os_memclear

  The os_memclear() clears memory to zero.

  Most often this function is not used, but C standard library function. Standard library function
  is much faster than this simple implementation. Search with OSAL_MEMORY_OS_CLR_AND_CPY string.

  @param   dst Pointer to memory to clear. If dst is OS_NULL, the function does nothing.
  @param   count Number of bytes to clear. If bytes zero or negative, the function does nothing.

  @return  None.

****************************************************************************************************
*/
void os_memclear(
    void *dst,
    os_memsz count)
{
	register os_char *d;

	if (dst == OS_NULL) return;
	d = dst;

	while (count-- > 0)
	{
		*(d++) = '\0';
	}
}

#endif
