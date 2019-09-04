/**

  @file    utf32/common/osal_char32.h
  @brief   Character classification and conversion.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This header file contains macros and function prototypes for character classification
  and conversion.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_CHAR32_INCLUDED
#define OSAL_CHAR32_INCLUDED

#if OSAL_UTF8
/** 
****************************************************************************************************

  @name UTF8 - UTF32 Conversion Functions

  The osal_char_utf32_to_utf8() function converts an UTF32 character to UTF8 encoding. This
  results from 1 to six bytes. The osal_char_utf8_to_utf32() converts character in UTF8
  encoding to UTF32 character.

****************************************************************************************************
 */
/*@{*/

/* Convert an UTF32 character to UTF8 encoding.
 */
os_int osal_char_utf32_to_utf8(
    os_char *buf,
    os_memsz buf_sz,
    os_uint c32);

/* Convert an UTF8 character to UTF32 character.
 */
os_uint osal_char_utf8_to_utf32(
    const os_char **c8ptr);

/*@}*/

#endif



#endif
