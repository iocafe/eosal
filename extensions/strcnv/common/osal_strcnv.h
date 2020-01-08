/**

  @file    strcnv/common/osal_strcnv.h
  @brief   Conversions between numbers and strings.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This osal_strcnv module header file. This module includes conversions between floating 
  point numbers, 64 bit integers and strings.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#if OSAL_STRCONV_SUPPORT

/** Flags for osal_double_to_str() function.
 */
#define OSAL_FLOAT_DEFAULT 0
#define OSAL_FLOAT_E_FORMAT 1

/* Convert floating point number to string.
 */
os_memsz osal_double_to_str(
    os_char *buf, 
    os_memsz buf_sz,
    os_double x, 
    os_int ddigs,
	os_int flags);

/* Convert string to floating point number.
 */
os_double osal_str_to_double(
    const os_char *str,
    os_memsz *count);

/* #define osal_int64_to_str(b,s,x) osal_int_to_str((b),(s),*(x)) */

/* Convert a string to 64 bit integer.
 */
os_memsz osal_str_to_int64(
    os_int64 *x, 
    os_char *str);

/* Convert 64 bit integer to string. If operating system supports 64 bit integers as 
   os_long type, we just use macro to map this to osal_int_to_str() function. 
   If operating system has no 64 bit support, the function implementation is used.
 */
#if OSAL_LONG_IS_64_BITS
    #define osal_int64_to_str(b,s,x) osal_int_to_str((b),(s),*(x))
#else
    os_memsz osal_int64_to_str(
		os_char *buf, 
        os_memsz buf_sz,
        os_int64 *x);
#endif

#endif
