/**

  @file    typeid/common/osal_typeid.h
  @brief   Enumeration of data types and type name - type id conversions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  This is typeid module header file. This module enumerates data types and implements functions
  for converting type name (text) to type identifier (integer) and vice versa, plus function
  to get type size in bytes. To use type enumeration only, just include this header file.
  If also functions are needed, link with the typeid library.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_TYPEID_H_
#define OSAL_TYPEID_H_
#include "eosalx.h"

/** Enumeration of type identifiers. Type identifiers are used to save and transfer data
    type information.
 */
typedef enum
{
    /** Undefined type.
     */
    OS_UNDEFINED_TYPE = 0,

    /** 8 bit signed character. In practise this is always same as char, one byte.
     */
    OS_BOOLEAN = 1,

    /** 8 bit signed character. In practise this is always same as char, one byte.
     */
    OS_CHAR = 2,

    /** @brief 8 bit unsigned character. In practise this is always same as unsigned char.
     */
    OS_UCHAR = 3,

    /** 16 bit signed integer. In practise os_short is always same as short.
     */
    OS_SHORT = 4,

    /** 16 bit unsigned integer. In practise os_ushort is always same as unsigned short.
     */
    OS_USHORT = 5,

    /** 32 bit signed integer. The os_int is ususally same as int, if platform's int is 32 bit.
        If platforms integer is only 16 bit, this is defined as long.
     */
    OS_INT = 6,

    /** 32 bit unsigned integer. The os_uint is ususally same as unsigned int, if platform's
        integers are 32 bit. If platforms integer is only 16 bit, this is defined as unsigned
        long.
     */
    OS_UINT = 7,

    /** Guaranteed 64 bit integer on all operating systems. If operating system supports
        64 bit integers, the operating system type is used. Otherwise implemented as structure.
     */
    OS_INT64 = 8,

    /** 64 bit signed integer. If OS/compiler doesn't support 64 integers, then 32 bit integer.
        For Microsoft compilers this is "__int64" and for GNU compilers "long long". The
        OSAL_LONG_IS_64_BITS define is checked so that embedded code without 64 bit support
        can be tested on Windows by setting the define to zero.
     */
    OS_LONG = 9,

    /** Single precision floating point number. In most cases this is same as float,
        typically 4 bytes.
     */
    OS_FLOAT = 10,

    /** Double precision floating point number. In most cases this is same as double,
        typically 4 bytes.
     */
    OS_DOUBLE = 11,

    /** Fixed point decimal number with one decimal digit. From -3276.8 to 3276.7.
     */
    OS_DEC01 = 12,

    /** Fixed point decimal number with two decimal digits. From -327.68 to 327.67.
     */
    OS_DEC001 = 13,

    /** String type.
     */
    OS_STR = 14,

    /** Object type.
     */
    OS_OBJECT = 15,

    /** Pointer type.
     */
    OS_POINTER = 16
}
osalTypeId;

/** Mask for getting type ID only, in case some other bits are stored in same integer.
 */
#define OSAL_TYPEID_MASK 0x1F

/** Macros to help to decide how to draw the type.
 */
#define OSAL_IS_UNDEFINED_TYPE(id) ((os_int)(id) == (os_int)OS_UNDEFINED_TYPE)
#define OSAL_IS_BOOLEAN_TYPE(id) ((os_int)(id) == (os_int)OS_BOOLEAN)
#define OSAL_IS_INTEGER_TYPE(id) ((os_int)(id) >= (os_int)OS_CHAR && (os_int)(id) <= (os_int)OS_LONG)
#define OSAL_IS_FLOAT_TYPE(id) ((os_int)(id) >= (os_int)OS_FLOAT && (os_int)(id) <= (os_int)OS_DEC001)

/** If type ID is stored in byte, it takes 5 bits. There are three extra bits which can
    be used on something else:
 */
#define OSAL_TYPEID_EXTRA_BIT_A 0x20
#define OSAL_TYPEID_EXTRA_BIT_B 0x40
#define OSAL_TYPEID_EXTRA_BIT_C 0x80

#if OSAL_TYPEID_SUPPORT

/* Convert type name string to type identifier (integer).
 */
osalTypeId osal_typeid_from_name(
    const os_char *name);

/* Convert type identifier to type name string.
 */
const os_char *osal_typeid_to_name(
    osalTypeId type_id);

#endif

#if OSAL_TYPE_RANGE_SUPPORT

/* Get numeric range of a type.
 */
void osal_type_range(
    osalTypeId type_id,
    os_long *min_value,
    os_long *max_value);

#endif

/* Get type size in bytes.
 */
os_memsz osal_type_size(
    osalTypeId type_id);

#endif
