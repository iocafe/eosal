/**

  @file    defs/common/osal_common_defs.h
  @brief   Micellenous defines common to all operating systems.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  This file contains micellenous defines, like OS_NULL, OS_TRUE..., which are common
  to all operating systems.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

/** Security and testing is difficult with security on, define to turn much of it off.
 */
#ifndef EOSAL_RELAX_SECURITY
  #define EOSAL_RELAX_SECURITY 0
#endif

/** Null pointer.
 */
#ifndef OS_NULL
#define OS_NULL 0
#endif

/** Value indication boolead condition TRUE.
 */
#define OS_TRUE 1

/** Value indication boolead condition FALSE.
 */
#define OS_FALSE 0

/** Boolean type
 */
typedef os_char os_boolean;

/** Timer value type
 */
#if OSAL_TIMER_IS_64_BITS
#define os_timer os_int64
#else
#define os_timer os_uint
#endif

/** Default OS path buffer size in bytes.
 */
#ifndef OSAL_PATH_SZ
#define OSAL_PATH_SZ 128
#endif

/** C++ compilation: The OSAL_C_HEADER_BEGINS marks beginning of header file text,
    which is to be left undecorated. Undecorated code is used to call C from inside
    C++ code. If compiling a C file, the OSAL_C_HEADER_BEGINS does nothing.
 */
#ifndef OSAL_C_HEADER_BEGINS
#ifdef __cplusplus
#define OSAL_C_HEADER_BEGINS extern "C" {
#else
#define OSAL_C_HEADER_BEGINS
#endif
#endif

/** C++ compilation: The OSAL_C_HEADER_ENDS marks beginning of header file text,
    which is to be left undecorated. See OSAL_C_HEADER_BEGINS. If compiling
    a C file, the OSAL_C_HEADER_ENDS does nothing.
 */
#ifndef OSAL_C_HEADER_ENDS
#ifdef __cplusplus
#define OSAL_C_HEADER_ENDS }
#else
#define OSAL_C_HEADER_ENDS
#endif
#endif

/** If we have no interrupt handler function type for this operating system,
    define empty OS_ISR_FUNC_ATTR.
 */
#ifndef OS_ISR_FUNC_ATTR
#define OS_ISR_FUNC_ATTR
#endif
#ifndef OS_ISR_DATA_ATTR
#define OS_ISR_DATA_ATTR
#endif
