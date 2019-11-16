/**

  @file    debug/common/osal_debug.h
  @brief   Debug related code.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Debug related code. When OSAL is compiled with nonzero OSAL_DEBUG flag, the code to detect
  program errors is included in compilation. If a programming error is detected, the
  osal_debug_error() function will be called. To find cause of a programming error, place
  a breakpoint within osal_debug error. When debugger stops to the breakpoint follow
  call stack to find the cause.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_DEBUG_INCLUDED
#define OSAL_DEBUG_INCLUDED


/** Define OSAL_DEBUG_FILE_AND_LINE as 1 to include file name and line number in osal_debug 
    and osal_assert macros. Effective only when OSAL_DEBUG is 1.
 */
#define OSAL_DEBUG_FILE_AND_LINE 1


/**
****************************************************************************************************

  @name Placeholder Macros

  If OSAL_DEBUG define is zero, these macros will replace debug functions and not generate any code.

****************************************************************************************************
*/
/*@{*/

#if OSAL_DEBUG == 0
  #define osal_debug_error(text)
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
	#define osal_debug_error(text) osal_debug_error_func(text, __FILE__, __LINE__)

	/* Macro to add file name line number to osal_debug_error_int_func() function arguments.
	 */
	#define osal_debug_error_int(text,v) osal_debug_error_int_func(text, v, __FILE__, __LINE__)

    /* Macro to add file name line number to osal_debug_error_str_func() function arguments.
     */
    #define osal_debug_error_str(text,v) osal_debug_error_str_func(text, v, __FILE__, __LINE__)

	/* Macro to add file name line number to osal_debug_assert_func() arguments.
	 */
    #define osal_debug_assert(cond) osal_debug_assert_func((os_long)(cond), (os_char*)__FILE__, __LINE__)

	/* Log a programming error.
	 */
	void osal_debug_error_func(
		const os_char *text,
        const os_char *file,
		os_int line);

	/* Log a programming error with integer argument v (appended).
	 */
	void osal_debug_error_int_func(
		const os_char *text,
        os_long v,
        const os_char *file,
		os_int line);

    /* Log a programming error with string argument v (appended).
     */
    void osal_debug_error_str_func(
        const os_char *text,
        const os_char *v,
        const os_char *file,
        os_int line);

	/* Report programming error if cond is zero.
	 */
	void osal_debug_assert_func(
		os_long cond, 
		os_char *file, 
		os_int line);

#else
	/* Log a programming error.
	 */
	void osal_debug_error(
		const os_char *text);

    /* Log a programming error with integer argument v (appended).
	 */
	void osal_debug_error_int(
		const os_char *text
        os_long v);

    /* Log a programming error with string argument v (appended).
     */
    void osal_debug_error_str(
        const os_char *text
        const os_char *v);

	/* Report programming error if cond is zero.
	 */
	void osal_debug_assert(
		os_long cond);
#endif

/*@}*/

#endif

/* Trace level 1 */
#if OSAL_TRACE
#define osal_trace(text) osal_debug_error(text)
#define osal_trace_int(text,v) osal_debug_error_int(text,v)
#define osal_trace_str(text,v) osal_debug_error_str(text,v)
#else
#define osal_trace(text)
#define osal_trace_int(text,v)
#define osal_trace_str(text,v)
#endif

/* Trace level 2 */
#if OSAL_TRACE >= 2
#define osal_trace2(text) osal_debug_error(text)
#define osal_trace2_int(text,v) osal_debug_error_int(text,v)
#define osal_trace2_str(text,v) osal_debug_error_str(text,v)
#else
#define osal_trace2(text)
#define osal_trace2_int(text,v)
#define osal_trace2_str(text,v)
#endif

/* Trace level 3 */
#if OSAL_TRACE >= 3
#define osal_trace3(text) osal_debug_error(text)
#define osal_trace3_int(text,v) osal_debug_error_int(text,v)
#define osal_trace3_str(text,v) osal_debug_error_str(text,v)
#else
#define osal_trace3(text)
#define osal_trace3_int(text,v)
#define osal_trace3_str(text,v)
#endif

#endif
