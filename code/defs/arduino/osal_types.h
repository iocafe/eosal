/**

  @file    defs/arduino/osal_types.h
  @brief   Definition of OS dependent primitive types for Linux.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This file defines operating system independent names for data types. 

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_TYPES_INCLUDED
#define OSAL_TYPES_INCLUDED

/** 
   @name Operating System Independent Types
   Since C doesn't fix type sizes, new names are defined for primitive types. This will
   help software to behave exactly the same way when run on different platforms.
 */
/*@{*/

/** 8 bit signed character. In practise this is always same as char, one byte.
    We use #define instead of typedef for character type, since enables debuggger
    to show whole string, not just the first character.
 */
#define os_char char
/* WAS: typedef char os_char; */

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
typedef long os_int;

/** 32 bit unsigned integer. The os_uint is ususally same as unsigned int, if platform's 
    integers are 32 bit. If platforms integer is only 16 bit, this is defined as unsigned 
	long.
 */
typedef unsigned long os_uint;

/** 64 bit signed integer. If OS/compiler doesn't support 64 integers, then 32 bit integer.
    For Microsoft compilers this is "__int64" and for GNU compilers "long long". The
	OSAL_LONG_IS_64_BITS define is checked so that embedded code without 64 bit support
	can be tested on Windows by setting the define to zero.
 */
#if OSAL_LONG_IS_64_BITS
typedef long long os_long;
#else
typedef long os_long;
#endif

/** The same as os_long, but unsigned.
 */
#if OSAL_LONG_IS_64_BITS
typedef unsigned long long os_ulong;
#else
typedef unsigned long os_ulong;
#endif

/** Memory size type. Define os_int 32 if maximum process memory space is <= 2GB, or
    os_long if more.
 */
typedef os_int os_memsz;

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
   For now many are missing.
 */
/*@{*/
#define OS_CHAR_MAX (~(os_schar)0 >> 1)
#define OS_SHORT_MAX (~(os_short)0 >> 1)
#define OS_INT_MAX (~(os_int)0 >> 1)
#define OS_LONG_MAX (~(os_long)0 >> 1)
#define OS_FLOAT_MAX 3.402823e+38
#define OS_DOUBLE_MAX 1.7976931348623158e+308
/*@}*/


#endif
