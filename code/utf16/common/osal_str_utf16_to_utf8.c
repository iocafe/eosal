/**

  @file    utf16/common/osal_str_utf16_to_utf8.c
  @brief   String conversion from UTF16 to UTF8.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Conversion of UTF16 encoded string to UTF8 encoded string, or necessary UTF8 buffer
  size calculation for conversion.

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

  @brief Convert an UTF16 string to UTF8 string.
  @anchor osal_str_utf16_to_utf8

  The osal_str_utf16_to_utf8() converts an UTF16 encoded string to UTF8 encoded string,
  or calculates UTF8 string buffer size needed to store UTF16 string given as argument.

  @param   buf8 Pointer to buffer into which to store resulting string. If the buffer is not
           large enough to store the whole string, the result is trunkated. In any case resulting
           string will be null terminated. This can be set to OS_NULL to calculate size
           of UTF8 buffer to store converted UTF16 string.
  @param   buf8_sz Buffer size in bytes. Ignored when size counting.
  @param   str16 Pointer to UTF16 encoded null terminated string. If OS_NULL, the function
           works as if str16 was en empty string "".

  @return  If size counting (buf8 is OS_NULL): Returns number of bytes needed to store the
           resulting UTF8 string including the terminating null character.
           Actual conversion, not size counting: Returns number of bytes actually stored to
           destination buffer buf8 including the terminating null character.

****************************************************************************************************
*/
os_memsz osal_str_utf16_to_utf8(
    os_char *buf8,
    os_memsz buf8_sz,
    const os_ushort *str16)
{
    os_uint c32;
    os_memsz pos;
    static os_ushort null_16 = '\0';

#if OSAL_UTF8
    os_int n;
#endif

    /* Treat NULL pointer source string as an empty string.
     */
    if (str16 == OS_NULL)
    {
        str16 = &null_16;
    }

    /* Convert the string. First destination character position is 0.
     */
    pos = 0;
    do
    {
        /* Get the first word. If we may need a second word, use the function (rare).
           This will move the pointer. Or if one word only, just increment the pointer.
         */
        c32 = *str16;
        if (c32 >= 0xD800) c32 = osal_char_utf16_to_utf32(&str16);
        else str16++;

#if OSAL_UTF8
        /* If this is not plain ASCII character, it should generate
           2 or more bytes. Use function.
         */
        if (c32 >= 0x80)
        {
            n = osal_char_utf32_to_utf8(buf8 ? buf8 + pos : OS_NULL,
                buf8_sz - pos - 1, c32);

            if (n == 0) goto getout;
            pos += n;
        }

        /* Plain ASCII character: If we have a buffer, check for space, save the
           character and increment the position. If no buffer, just increment
           destination position.
         */
        else if (buf8)
        {
            if (pos+1 >= buf8_sz) goto getout;
            buf8[pos++] = c32;
        }
        else
        {
            pos++;
        }
#else
        if (buf8)
        {
            if (pos+1 >= buf8_sz) goto getout;
            buf8[pos++] = c32;
        }
        else
        {
            pos++;
        }
#endif
    }
    while (c32 != '\0');

    /* Return number of bytes stored.
     */
    return pos;

    /* Get out when error.
     */
getout:
    if (buf8 && pos < buf8_sz)
    {
        buf8[pos] = '\0';
    }
    return pos + 1;
}

#endif
