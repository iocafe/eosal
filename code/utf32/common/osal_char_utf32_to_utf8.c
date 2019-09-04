/**

  @file    utf32/common/osal_char_utf32_to_utf8.c
  @brief   Character conversion from UTF32 to UTF8.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Conversion of UTF32 character to UTF8 character string encoding. The UTF8 character may take
  from 1 to 6 bytes to store. If we need UTF8 support (define OSAL_UTF8 is nonzero), we need
  to be convert between UTF8 and UTF32 characters.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"

/* If we need UTF8 support, we need to be ablt to convert between UTF8 and UTF32 characters.
 */
#if OSAL_UTF8

/**
****************************************************************************************************

  @brief Convert an UTF32 character to UTF8 encoding.
  @anchor osal_char_utf32_to_utf8

  The osal_char_utf32_to_utf8() converts an UTF32 character to UTF8 character. UTF8 character
  may take from 1 to 6 bytes to store. This function can be used also only determine how many
  bytes it takes to store a UTF32 character in UTF8 encoding, without storing anything.

  @param   buf Buffer into which to store the character in UTF8 encoding. The result will NOT
           be terminated by null character. If you need to null terminate buffer, use return
           value rval of this function, and set buf[rval] = 0.
           If buf argument is OS_NULL, the function does size counting. The buf_sz argument
           is ignored and the function just returns number of bytes needed to store the character.
  @param   buf_sz Buffer size. This is maximum number of bytes to store into buffer.
  @param   c32 UTF32 character to convert.

  @return  If the function succeeds, it returns number of bytes (1-6) stored into buffer
           (or number of bytes that would be needed if buf is OS_NULL). If the function
           fails to convert the character either becouse given buffer is too small or
           character c32 is not legimate unicode character, the function returns 0.

****************************************************************************************************
*/
os_int osal_char_utf32_to_utf8(
    os_char *buf,
    os_memsz buf_sz,
    os_uint c32)
{
    os_uint sz, limit_value, sh, mask;

    /* One byte UTF8 character. This is most common case.
     */
    if (c32 < 0x80)
    {
        if (buf == OS_NULL) return 1;
        if (buf_sz < 1) return 0;
        *buf = (os_char)c32;
        return 1;
    }

    /* From two to six bytes. Decide on number of bytes by value.
       Return 0 to indicate an error if more that 6 bytes.
     */
    limit_value = 0x800;
    sz = 2;
    while (c32 >= limit_value)
    {
        if (++sz > 6) return 0;
        limit_value <<= 5;
    }

    /* If we are doing only size counting, just return the size.
     */
    if (buf == OS_NULL) return sz;

    /* If doesn't fit to buffer given as argument, return error.
    */
    if ((os_long)sz > buf_sz) return 0;

    /* Calculate mask and shift for first byte.
     */
    mask = (0xFC << (6-sz));
    sh = (sz-1) * 6;

    /* Save the first byte.
     */
    *(buf++) = (os_char)((c32 >> sh) | mask);
    sh -= 6;

    /* Save middle bytes.
     */
    while (sh)
    {
        *(buf++) = (os_char)(((c32 >> sh)&0x3F) | 0x80);
        sh -= 6;
    }

    /* Save the last byte.
     */
    *buf = (os_char)((c32 & 0x3F) | 0x80);

    /* Return size.
     */
    return sz;
}

#endif
