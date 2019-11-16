/**

  @file    utf32/common/osal_char_utf8_to_utf32.c
  @brief   Character conversion from UTF8 to UTF32.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Conversion of UTF8 character to UTF32 character. The UTF8 character to convert may take
  from 1 to 6 bytes, while the UTF32 character is always 32 bit unsigned integer.
  If we need UTF8 support (define OSAL_UTF8 is nonzero), we need to be convert between
  UTF8 and UTF32 characters.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
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

  @brief Convert an UTF8 character to UTF32 character.
  @anchor osal_char_utf8_to_utf32

  The osal_char_utf8_to_utf32() converts an UTF8 character to UTF32 character. The UTF8 character
  takes from 1 to 6 bytes to store.

  @param   c8ptr Pointer to pointer to lead byte of UTF8 character. This pointer is advanced
           by number of bytes processed from the UTF8 character.

  @return  If the function succeeds, it returns UTF32 character, and moves c8ptr by number
           of bytes processed. The function fails if the *c8ptr doesn't point to lead byte
           of valid UTF8 character. In this case function returns 0 and moves
           c8ptr by one.

****************************************************************************************************
*/
os_uint osal_char_utf8_to_utf32(
    const os_char **c8ptr)
{
    os_uchar *c, firstc;
    os_uint mask, sz, c32;

    c = (os_uchar*)*c8ptr;

#if OSAL_DEBUG
    if (c == OS_NULL)
    {
        osal_debug_error("NULL argument");
        return 0;
    }
#endif

    /* First character.
     */
    firstc = *c;

    /* Single byte
     */
    if ((firstc & 0x80) == 0)
    {
        ++(*c8ptr);
        return firstc;
    }

    /* If this is not a lead byte, just quit.
     */
    if ((firstc & 0x40) == 0) goto getout;

    /* Get number of bytes
     */
    mask = 0x20;
    sz = 2;
    while (firstc & mask)
    {
        if (++sz > 6) goto getout;
        mask >>= 1;
    }

    /* Process the first byte.
     */
    c32 = firstc & (0x7F >> sz);

    /* Process the rest
     */
    while (--sz)
    {
        c32 <<= 6;
        c32 |= (0x3F & *(++c));
    }

    *c8ptr = (os_char*)c;
    return c32;

getout:
    *c8ptr += 1;
    return 0;
}

#endif
