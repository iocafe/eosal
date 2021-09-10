/**

  @file    defs/windows/osal_types.h
  @brief   OS independent primitive types for Windows.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  This file defines operating system independent names for data types.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_TYPES_H_
#define OSAL_TYPES_H_
#include "eosal.h"

/**
   @name Operating System Independent Types
   Since C doesn't fix type sizes, new names are defined for primitive types. This will
   help software to behave exactly the same way when run on different platforms.
 */
/*@{*/

/** 8 bit signed character. In practise this is always same as char, one byte.
 */
typedef char os_char;

/** 8 bit signed character. This is forced to be always signed. Use this for type conversions
    instead of os_char.
 */
typedef signed char os_schar;

/** @brief 8 bit unsigned character. In practise this is always same as unsigned char.
 */
typedef unsigned char os_uchar;

/** 16 bit signed integer. In practise os_short is always same as short.
 */
typedef short os_short;

/** 16 bit unsigned integer. In practise os_ushort is always same as unsigned short.
 */
typedef unsigned short os_ushort;

/** 32 bit signed integer. The os_int is ususally same as int, if platform's int is 32 bit.
    If platforms integer is only 16 bit, this is defined as long.
 */
typedef int os_int;

/** 32 bit unsigned integer. The os_uint is ususally same as unsigned int, if platform's
    integers are 32 bit. If platforms integer is only 16 bit, this is defined as unsigned
    long.
 */
typedef unsigned int os_uint;

/** Long signed integer. If OS/compiler doesn't support 64 integers, then 32 bit integer.
    32 bit long type can be used on minimalistic 8/16 bit microcontrollers to save memory.
 */
#if OSAL_LONG_IS_64_BITS
typedef long long os_long;
#else
typedef int os_long;
#endif

/** The same as os_long, but unsigned.
 */
#if OSAL_LONG_IS_64_BITS
typedef unsigned long long os_ulong;
#else
typedef unsigned int os_ulong;
#endif

/** 64 bit signed integer. If OS/compiler doesn't support 64 integers, then os_longlong is
    undefined. (The os_longlong/os_ulonglong should not be used in application.
    Use type os_int64 if you need always 64 bit byte)
    For older Microsoft compilers this is "__int64" and for GNU and most new compilers "long long".
 */
#if OSAL_COMPILER_HAS_64_BIT_INTS
typedef long long os_longlong;
typedef unsigned long long os_ulonglong;
#endif

/** Memory size type. Define os_int 32 if maximum process memory space is <= 2GB, or
    os_long if more. This must be a signed integer type.
 */
typedef os_long os_memsz;

/** Single precision floating point number. In most cases this is same as float,
    typically 4 bytes.
 */
typedef float os_float;

/** Double precision floating point number. In most cases this is same as double,
    typically 4 bytes.
 */
typedef double os_double;

/** Pointer type, often function pointer.
 */
typedef void *os_pointer;

/*@}*/

/**
   @name Limits for types
   Maximum and minimum values for each type should be defined here.
   We use constants. It is important that the limits are same for
   all hardware architectures. Notes:
   - Rare microcontroller environments have no compiler 64 bit integer
     support -> the OS_LONG_MIN and OS_LONG_MAX will not work. Avoid using
     these in code which needs to run in such environments.
   - Avoid using OS_FLOAT_MAX and OS_DOUBLE_MAX. These are floating point
     hardware specififc (values here work for most, but not all).
   - For now some defines are missing. These may be added later.
 */
/*@{*/
#define OS_CHAR_MAX 127
#define OS_SHORT_MAX 32767
#define OS_INT_MAX ((os_int)0x7FFFFFFF)
#define OS_LONG_MAX ((os_long)0x7FFFFFFFFFFFFFFFLL)
#define OS_CHAR_MIN -128
#define OS_SHORT_MIN -32768
#define OS_INT_MIN ((os_int)0x80000000)
#define OS_LONG_MIN ((os_long)0x8000000000000000ULL)

#define OS_UCHAR_MAX 255
#define OS_USHORT_MAX 65535
#define OS_UINT_MAX ((os_uint)0xFFFFFFFF)

#define OS_FLOAT_MAX 3.402823e+38F
#define OS_DOUBLE_MAX 1.7976931348623158e+308
/*@}*/

#endif
