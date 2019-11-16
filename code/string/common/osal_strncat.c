/**

  @file    string/commmon/osal_strncat.c
  @brief   Append a string to another string.
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


/**
****************************************************************************************************

  @brief Append a string to another string.
  @anchor os_strncat

  The os_strncat() function appends the src string to dst string. Resulting string is 
  terminated by null character, even whole string would not fit into destination buffer.

  @param   dst Destination pointer.
  @param   src Source pointer.
  @param   dst_size Size of destination buffer in bytes.

  @return  None.

****************************************************************************************************
*/
void os_strncat(
    os_char *dst,
    const os_char *src,
    os_memsz dst_size)
{
	os_char *d;

	/* If we can do nothing.
	 */
	if (dst == OS_NULL || dst_size <= 1 || src == OS_NULL) return;

	/* Find terminating null character. 
	 */
	d = dst;
	while (*d) d++;

	/* Substracy number of used bytes from dst_size.
	 */
	dst_size -= (d - dst);
	if (dst_size <= 1) return;

	/* Append string until null character found, or out of destination buffer (space left for 
	   null character only).
	 */
	while (dst_size-- > 1 && *src)
	{
		*(d++) = *(src++);
	}

	/* Terminate with null character
	 */
	*(d++) = '\0';
}
