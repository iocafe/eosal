/**

  @file    typeid/common/osal_typeid.h
  @brief   Enumeration of data types and type name - type id conversions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This is typeid module header file. This module enumerates data types and implements functions
  for converting type name (text) to type identifier (integer) and vice versa, plus function
  to get type size in bytes. To use type enumeration only, just include this header file. 
  If also functions are needed, link with the typeid library.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_TYPEID_INCLUDED
#define OSAL_TYPEID_INCLUDED
#if OSAL_TYPEID_SUPPORT

/* If C++ compilation, all functions, etc. from this point on in this header file are
   plain C and must be left undecorated.
 */
OSAL_C_HEADER_BEGINS

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
	OS_CHAR = 1,

	/** @brief 8 bit unsigned character. In practise this is always same as unsigned char.
	 */
	OS_UCHAR = 2,

	/** 16 bit signed integer. In practise os_short is always same as short.
	 */
	OS_SHORT = 3,

	/** 16 bit unsigned integer. In practise os_ushort is always same as unsigned short.
	 */
	OS_USHORT = 4,

	/** 32 bit signed integer. The os_int is ususally same as int, if platform's int is 32 bit.
		If platforms integer is only 16 bit, this is defined as long.
	 */
	OS_INT = 5, 

	/** 32 bit unsigned integer. The os_uint is ususally same as unsigned int, if platform's 
		integers are 32 bit. If platforms integer is only 16 bit, this is defined as unsigned 
		long.
	 */
	OS_UINT = 6,

	/** Guaranteed 64 bit integer on all operating systems. If operating system supports 
	    64 bit integers, the operating system type is used. Otherwise implemented as structure. 
	 */
	OS_INT64 = 7,

	/** 64 bit signed integer. If OS/compiler doesn't support 64 integers, then 32 bit integer.
		For Microsoft compilers this is "__int64" and for GNU compilers "long long". The
		OSAL_LONG_IS_64_BITS define is checked so that embedded code without 64 bit support
		can be tested on Windows by setting the define to zero.
	 */
	OS_LONG = 8,

	/** Single precision floating point number. In most cases this is same as float, 
		typically 4 bytes.
	 */
	OS_FLOAT = 9,

	/** Double precision floating point number. In most cases this is same as double, 
		typically 4 bytes.
	 */
	OS_DOUBLE = 10,

	/** Fixed point decimal number with one decimal digit. From -3276.8 to 3276.7.
	 */
	OS_DEC01 = 11,

	/** Fixed point decimal number with two decimal digits. From -327.68 to 327.67.
	 */
	OS_DEC001 = 12,

	/** String type.
	 */
	OS_STRING = 13,

	/** Object type.
	 */
	OS_OBJECT = 14,

	/** Pointer type.
	 */
	OS_POINTER = 15
}
osalTypeId;

/* Convert type name string to type identifier (integer).
 */
osalTypeId osal_typeid_from_name(
    os_char *name);

/* Convert type identifier to type name string.
 */
os_char *osal_typeid_to_name(
    osalTypeId type_id);

/* Get type size in bytes.
 */
os_memsz osal_typeid_size(
    osalTypeId type_id);


/* If C++ compilation, end the undecorated code.
 */
OSAL_C_HEADER_ENDS

#endif
#endif
