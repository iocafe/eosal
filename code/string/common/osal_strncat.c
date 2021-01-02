/**

  @file    string/commmon/osal_strncat.c
  @brief   Append a string to another string.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
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

  @return  OSAL_SUCCESS if wole string was appended. Return value OSAL_STATUS_OUT_OF_BUFFER
           indicates running out of destination buffer. If out of buffer, partial string
           may be appended.

****************************************************************************************************
*/
osalStatus os_strncat(
    os_char *dst,
    const os_char *src,
    os_memsz dst_size)
{
    os_char *d;

    if (src == OS_NULL) {
        return OSAL_SUCCESS;
    }
    if (*src == '\0') {
        return OSAL_SUCCESS;
    }

    /* If we can do nothing.
     */
    if (dst == OS_NULL || dst_size <= 1) {
        return OSAL_STATUS_OUT_OF_BUFFER;
    }

    /* Find terminating null character and substract number of used bytes from dst_size.
     */
    d = dst;
    while (*d) {
        d++;
        if (--dst_size <= 1) {
            return OSAL_STATUS_OUT_OF_BUFFER;
        }
    }

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

    /* Return OSAL_SUCCESS if whole source string was appended.
     */
    return *src == '\0' ? OSAL_SUCCESS : OSAL_STATUS_OUT_OF_BUFFER;
}
