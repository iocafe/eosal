/**

  @file    utf16/common/osal_char_utf32_to_utf16.c
  @brief   Character conversion from UTF32 to UTF16.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Conversion of UTF32 character to UTF16 character. The UTF16 character takes 1 or 2 words
  (osal_ushorts) to store.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"

/* If we need UTF16 support (windows), we need to be ablt to convert between UTF16 and UTF32 
   characters.
 */
#if OSAL_UTF16

/**
****************************************************************************************************

  @brief Convert an UTF32 character to UTF8 encoding.
  @anchor osal_char_utf32_to_utf16

  The osal_char_utf32_to_utf16() converts an UTF32 character to UTF16 character. UTF16 character
  takes 1 or 2 words to store. This function can be used also only determine how many words it
  takes to store a UTF32 character in UTF16 encoding, without storing anything.

  @param   buf Buffer into which to store the character in UTF16 encoding. The result will NOT
           be terminated by null character. If you need to null terminate buffer, use return
           value rval of this function, and set buf[rval] = 0.
           If buf argument is OS_NULL, the function does size counting. The buf_n argument
           is ignored and the function just returns number of words needed to store the character.
  @param   buf_n Buffer size in words. This is maximum number of words to store into buffer.
  @param   c32 UTF32 character to convert.

  @return  If the function succeeds, it returns number of words (1 or 2) stored into buffer
           (or number of words that would be needed if buf is OS_NULL). If the function
           fails to convert the character either becouse given buffer is too small or
           character c32 is not legimate unicode character, the function returns 0.

****************************************************************************************************
*/
os_int osal_char_utf32_to_utf16(
    os_ushort *buf,
    os_memsz buf_n,
    os_uint c32)
{
    /* If this can be stored as single word UTF16 character.
     */
    if (c32 <= 0xFFFF)
    {
        /* Unicode points from D800 to DBFF (1,024 code points) are known as high-surrogate
           code points and DC00 to DFFF (1,024 code points) as low surrogare code point.
           These are reserved for UTF16 two word surrogate pairs cannot be real UTF characters.
         */
        if ((c32 & 0xFC00) == 0xD800) goto getout;

        /* If size counting only.
         */
        if (buf == OS_NULL) return 1;

        /* If no space in buffer.
         */
        if (buf_n <= 0) goto getout;

        /* Save the character as one word, and return 1 to indicate one word
           character.
         */
        *buf = (os_ushort)c32;
        return 1;
    }

    /* Two words needed to represent character in UTF16.
     */
    if (c32 <= 0x10FFFFL)
    {
        /* If size counting only.
         */
        if (buf == OS_NULL) return 2;

        /* If no space in buffer.
         */
        if (buf_n <= 1) goto getout;

        /* Subtract 0x10000 from code point value and store into two 10 bit words,
           more significant part first. Return 2 two indicate that two words were
           needed to represent the character.
         */
        c32-=0x10000L;
        *buf = (os_ushort)((c32 >> 10) | 0xD800);
        buf[1] = (os_ushort)((c32 & 0x3FF) | 0xDC00);
        return 2;
    }

getout:
    return 0;
}

#endif
