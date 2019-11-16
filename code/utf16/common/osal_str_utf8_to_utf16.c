/**

  @file    utf16/common/osal_str_utf8_to_utf16.c
  @brief   String conversion from UTF8 to UTF16.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Conversion of UTF8 encoded string to UTF16 encoded string, or necessary UTF16 buffer
  length calculation for conversion.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
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

  @brief Convert an UTF8 string to UTF16 string.
  @anchor osal_str_utf8_to_utf16

  The osal_str_utf8_to_utf16() converts an UTF8 encoded string to UTF16 encoded string,
  or calculates UTF16 string buffer length in words, which is needed to store UTF8 string
  given as argument.

  @param   buf16 Pointer to buffer into which to store resulting UTF16 string. If the buffer is
           not large enough to store the whole string, the result is trunkated. In any case the
           resulting string will be null terminated. This can be set to OS_NULL to calculate
           length of UTF16 buffer to store the converted UTF8 string.
  @param   buf16_n UTF16 buffer length in words. Ignored when size counting.
  @param   str8 Pointer to UTF8 encoded null terminated string. If OS_NULL, the function
           works as if str8 was en empty string "".

  @return  If size counting (buf16 is OS_NULL): Returns number of words needed to store the
           resulting UTF16 string including the terminating null character.
           Actual conversion, not size counting: Returns number of words actually stored to
           destination buffer buf16 including the terminating null character.

****************************************************************************************************
*/
os_memsz osal_str_utf8_to_utf16(
    os_ushort *buf16,
    os_memsz buf16_n,
    const os_char *str8)
{
    os_uint c32;
    os_int n;
    os_memsz pos;

    /* Treat NULL pointer source string as an empty string.
     */
    if (str8 == OS_NULL)
    {
        str8 = "";
    }

    /* Convert the string. First destination character position is 0.
     */
    pos = 0;
    do
    {
        /* Get the first byte. If we may need more bytes, use the function. This will move
           the pointer. Or if one ASCII byte, just increment the pointer.
         */
#if OSAL_UTF8
        c32 = *str8;
        if (c32 >= 0x80) c32 = osal_char_utf8_to_utf32(&str8);
        else str8++;
#else
        c32 = *(str8++);
#endif
        /* If this characters as UTF16 may need two words, use function.
         */
        if (c32 >= 0xD800)
        {
            n = osal_char_utf32_to_utf16(buf16 ? buf16 + pos : OS_NULL,
                buf16_n - pos - 1, c32);

            if (n == 0) goto getout;
            pos += n;
        }

        /* Single word: If we have a buffer, check for space, save the character
           and increment the position. If no buffer, just increment
           destination position.
         */
        else if (buf16)
        {
            if (pos+1 >= buf16_n) goto getout;
            buf16[pos++] = c32;
        }
        else
        {
            pos++;
        }
    }
    while (c32 != '\0');

    /* Return number of bytes stored.
     */
    return pos;

    /* Get out when error.
     */
getout:
    if (buf16 && pos < buf16_n)
    {
        buf16[pos] = '\0';
    }
    return pos + 1;
}

#endif
