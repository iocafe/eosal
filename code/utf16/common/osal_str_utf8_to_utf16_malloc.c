/**

  @file    utf16/common/osal_str_utf8_to_utf16_malloc.c
  @brief   String conversion from UTF8 to UTF16 into new allocated buffer.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Conversion of UTF8 encoded string to UTF16 encoded string, buffer for the new string
  is allocated by os_malloc() function.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"

/* If we need UTF16 support (windows), we need to be ablt to convert between UTF16 and UTF8 
   characters.
 */
#if OSAL_UTF16

/**
****************************************************************************************************

  @brief Convert an UTF8 string to UTF16 string in newly allocated buffer.
  @anchor osal_str_utf8_to_utf16_malloc

  The osal_str_utf8_to_utf16_malloc() converts an UTF16 encoded string to UTF8 encoded string.
  The UTF8 string is stored into buffer allocated using os_malloc() by this function.

  Code sniplet below demonstrates converting string, and then releasing the conversion buffer.
  \verbatim
  os_ushort *str;
  os_long sz;

  str = osal_str_utf8_to_utf16_malloc("My UTF8 string", &sz);
  os_free(str, sz);
  \endverbatim

  @param   str8 Pointer to UTF8 encoded null terminated string. If OS_NULL, the function
           works as if str8 was en empty string "".
  @param   sz_ptr Pointer to integer into which to store size of allocated buffer in bytes
           (really bytes, not words). OS_NULL if not needed.

  @return  Pointer to UTF16 string (Beginning of allocated memory block).

****************************************************************************************************
*/
os_ushort *osal_str_utf8_to_utf16_malloc(
    const os_char *str8,
    os_memsz *sz_ptr)
{
    os_memsz sz;
    os_ushort *buf16;

    /* Calculate buffer size to hold the UTF16 encoded string.
     */
    sz = osal_str_utf8_to_utf16(OS_NULL, 0, str8);

    /* Allocate buffer for the string.
     */
    buf16 = (os_ushort*)os_malloc(sz*sizeof(os_ushort), OS_NULL);

    /* Convert the string from UTF8 to UTF16.
     */
    osal_str_utf8_to_utf16(buf16, sz, str8);

    /* Return the buffer size if needed, and return the
       string pointer.
     */
    if (sz_ptr) *sz_ptr = sz*sizeof(os_ushort);
    return buf16;
}

#endif
