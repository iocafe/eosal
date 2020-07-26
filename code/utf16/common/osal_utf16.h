/**

  @file    utf16/common/osal_utf16.h
  @brief   UTF16 support.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  This OSAL utf16 module header file. This module includes conversions between UTF16 and
  UTF32 characters and between UTF16 and UTF8 strings. UTF-16 is the native character encoding
  for Windows, Java and .NET bytecode environments, Mac OS X's Cocoa and Core Foundation
  frameworks and the Nokia Qt. Thus the osal_utf16 module is practically always
  needed for these environments (OSAL uses UTF8 for ASCII compatibility on low end systems).

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_UTF16_H_
#define OSAL_UTF16_H_
#include "eosal.h"

/* If we need UTF16 support (Windows), include conversions between UTF16 and UTF8 characters.
 */
#if OSAL_UTF16

/* Convert UTF32 character to UTF16.
 */
os_int osal_char_utf32_to_utf16(
    os_ushort *buf,
    os_memsz buf_n,
    os_uint c32);

/* Convert UTF16 character to UTF32.
 */
os_uint osal_char_utf16_to_utf32(
    const os_ushort **c16ptr);

/* Convert an UTF16 string to UTF8 string.
 */
os_memsz osal_str_utf16_to_utf8(
    os_char *buf8,
    os_memsz buf8_sz,
    const os_ushort *str16);

/* Convert an UTF8 string to UTF16 string.
 */
os_memsz osal_str_utf8_to_utf16(
    os_ushort *buf16,
    os_memsz buf16_n,
    const os_char *str8);

/* Convert an UTF16 string to UTF8 string in newly allocated buffer.
 */
os_char *osal_str_utf16_to_utf8_malloc(
    const os_ushort *str16,
    os_memsz *sz_ptr);

/* Convert an UTF8 string to UTF16 string in newly allocated buffer.
 */
os_ushort *osal_str_utf8_to_utf16_malloc(
    const os_char *str8,
    os_memsz *sz_ptr);

#endif
#endif
