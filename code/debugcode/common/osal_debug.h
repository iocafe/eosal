/**

  @file    debug/common/osal_debug.h
  @brief   Debug related code.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Debug related code. When OSAL is compiled with nonzero OSAL_DEBUG flag, the code to detect
  program errors is included in compilation. If a programming error is detected, the
  osal_debug_error() function will be called. To find cause of a programming error, place
  a breakpoint within osal_debug error. When debugger stops to the breakpoint follow
  call stack to find the cause.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_DEBUG_H_
#define OSAL_DEBUG_H_
#include "eosal.h"


/**
****************************************************************************************************

  @name Placeholder Macros

  If OSAL_DEBUG define is zero, these macros will replace debug functions and not generate any code.

****************************************************************************************************
*/
/*@{*/

#if OSAL_DEBUG == 0
  #define osal_debug_error(text)
  #define osal_debug_error_int(text,v)
  #define osal_debug_error_str(text,v)
  #define osal_debug_assert(cond)
#endif
/*@}*/


/**
****************************************************************************************************

  @name Debug Functions

  These functions exists only when compiling with nonzero OSAL_DEBUG define. If the define
  is zero, empty macros will take palce of these functions.

****************************************************************************************************
 */
/*@{*/
#if OSAL_DEBUG

#if OSAL_DEBUG_FILE_AND_LINE
    /* Macro to add file name line number to osal_debug_error_func() function arguments.
     */
    #define osal_debug_error(text) osal_debug_error_func(text, __FILE__, __LINE__, OS_TRUE)
    #define osal_debug_trace(text) osal_debug_error_func(text, __FILE__, __LINE__, OS_FALSE)

    /* Macro to add file name line number to osal_debug_error_int_func() function arguments.
     */
    #define osal_debug_error_int(text,v) osal_debug_error_int_func(text, v, __FILE__, __LINE__, OS_TRUE)
    #define osal_debug_trace_int(text,v) osal_debug_error_int_func(text, v, __FILE__, __LINE__, OS_FALSE)

    /* Macro to add file name line number to osal_debug_error_str_func() function arguments.
     */
    #define osal_debug_error_str(text,v) osal_debug_error_str_func(text, v, __FILE__, __LINE__, OS_TRUE)
    #define osal_debug_trace_str(text,v) osal_debug_error_str_func(text, v, __FILE__, __LINE__, OS_FALSE)

    /* Macro to add file name line number to osal_debug_assert_func() arguments.
     */
    #define osal_debug_assert(cond) osal_debug_assert_func((os_long)(cond), (os_char*)__FILE__, __LINE__)

    /* Log a programming error.
     */
    void osal_debug_error_func(
        const os_char *text,
        const os_char *file,
        os_int line,
        os_boolean is_error);

    /* Log a programming error with integer argument v (appended).
     */
    void osal_debug_error_int_func(
        const os_char *text,
        os_long v,
        const os_char *file,
        os_int line,
        os_boolean is_error);

    /* Log a programming error with string argument v (appended).
     */
    void osal_debug_error_str_func(
        const os_char *text,
        const os_char *v,
        const os_char *file,
        os_int line,
        os_boolean is_error);

    /* Report programming error if cond is zero.
     */
    void osal_debug_assert_func(
        os_long cond,
        os_char *file,
        os_int line);

#else
    /* Macro to add file name line number to osal_debug_error_func() function arguments.
     */
    #define osal_debug_error(text) osal_debug_error_func(text, OS_TRUE)
    #define osal_debug_trace(text) osal_debug_error_func(text, OS_FALSE)

    /* Macro to add file name line number to osal_debug_error_int_func() function arguments.
     */
    #define osal_debug_error_int(text,v) osal_debug_error_int_func(text, v, OS_TRUE)
    #define osal_debug_trace_int(text,v) osal_debug_error_int_func(text, v, OS_FALSE)

    /* Macro to add file name line number to osal_debug_error_str_func() function arguments.
     */
    #define osal_debug_error_str(text,v) osal_debug_error_str_func(text, v, OS_TRUE)
    #define osal_debug_trace_str(text,v) osal_debug_error_str_func(text, v, OS_FALSE)

    /* Macro to add file name line number to osal_debug_assert_func() arguments.
     */
    #define osal_debug_assert(cond) osal_debug_assert_func((os_long)(cond))

    /* Log a programming error.
     */
    void osal_debug_error_func(
        const os_char *text,
        os_boolean is_error);

    /* Log a programming error with integer argument v (appended).
     */
    void osal_debug_error_int_func(
        const os_char *text,
        os_long v,
        os_boolean is_error);

    /* Log a programming error with string argument v (appended).
     */
    void osal_debug_error_str_func(
        const os_char *text,
        const os_char *v,
        os_boolean is_error);

    /* Report programming error if cond is zero.
     */
    void osal_debug_assert_func(
        os_long cond);
#endif

/*@}*/

#endif

/* Trace level 1 */
#if OSAL_TRACE
#define osal_trace(text) osal_debug_trace(text)
#define osal_trace_int(text,v) osal_debug_trace_int(text,v)
#define osal_trace_str(text,v) osal_debug_trace_str(text,v)
#else
#define osal_trace(text)
#define osal_trace_int(text,v)
#define osal_trace_str(text,v)
#endif

/* Trace level 2 */
#if OSAL_TRACE >= 2
#define osal_trace2(text) osal_debug_trace(text)
#define osal_trace2_int(text,v) osal_debug_trace_int(text,v)
#define osal_trace2_str(text,v) osal_debug_trace_str(text,v)
#else
#define osal_trace2(text)
#define osal_trace2_int(text,v)
#define osal_trace2_str(text,v)
#endif

/* Trace level 3 */
#if OSAL_TRACE >= 3
#define osal_trace3(text) osal_debug_trace(text)
#define osal_trace3_int(text,v) osal_debug_trace_int(text,v)
#define osal_trace3_str(text,v) osal_debug_trace_str(text,v)
#else
#define osal_trace3(text)
#define osal_trace3_int(text,v)
#define osal_trace3_str(text,v)
#endif

#endif
