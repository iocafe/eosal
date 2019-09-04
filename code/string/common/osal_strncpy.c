/**

  @file    string/commmon/osal_strncpy.c
  @brief   Copy memory.
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


/**
****************************************************************************************************

  @brief Copy string.
  @anchor os_strncpy

  The os_strncpy() function copies count bytes of src to dst. Copied string is terminated
  by null character, even whole source string would not fit into destination buffer.

  @param   dst Destination pointer.
  @param   src Source pointer.
  @param   dst_size Size of destination buffer. This is the maximum number of bytes to copy
		   including terminating null character.

  @return  None.

****************************************************************************************************
*/
void os_strncpy(
    os_char *dst,
    const os_char *src,
    os_memsz dst_size)
{
	/* If we can do nothing.
	 */
	if (dst == OS_NULL || dst_size <= 0) return;

	/* If source string is NULL pointer, act as if copying empry string.
	 */
	if (src == OS_NULL) src = "";

	/* Copy string until null character found, or out of destination buffer (space left for null 
	   character only).
	 */
	while (dst_size-- > 1 && *src)
	{
		*(dst++) = *(src++);
	}

	/* Terminate with null character
	 */
	*(dst++) = '\0';
}
