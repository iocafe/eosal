/**

  @file    utf16/common/osal_char_utf16_to_utf32.c
  @brief   Character conversion from UTF16 to UTF32.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Conversion of UTF16 character to UTF32 character. The UTF16 character takes either 1 or
  two 16 bit words, while the UTF32 character is always 32 bit unsigned integer.

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

  @brief Convert an UTF16 character to UTF32 character.
  @anchor osal_char_utf16_to_utf32

  The osal_char_utf16_to_utf32() converts an UTF16 character to UTF32 character.

  @param   c16ptr Pointer to first word of the UTF16 character. This pointer is advanced
           by number of words processed from the UTF8 character (either 1 or 2).

  @return  If the function succeeds, it returns UTF32 character, and moves c16ptr by number
           of words processed. If the function fails, it returns 0 and moves c16ptr by one.

****************************************************************************************************
*/
os_uint osal_char_utf16_to_utf32(
    const os_ushort **c16ptr)
{
    os_uint a, b;

#if OSAL_DEBUG
    if (*c16ptr == OS_NULL)
    {
        osal_debug_error("NULL ptr");
        return 0;
    }
#endif

    /* First word (usually the only one)
     */
    a = *((*c16ptr)++);

    /* If this is first part of surrogate pair.
       Unicode points from D800 to DBFF (1,024 code points) are known as high-surrogate
       code points and DC00 to DFFF (1,024 code points) as low surrogare code point.
       These are reserved for UTF16 two word surrogate pairs cannot be real UTF characters.
     */
    if ((a & 0xFC00) == 0xD800)
    {
        b = *((*c16ptr)++);
        return (((a & 0x3F)<<10) | (b&0x3F)) + 0x10000;
    }

    /* Just one word character.
     */
    return a;
}

#endif
