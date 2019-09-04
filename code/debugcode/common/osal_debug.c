/**

  @file    debugcode/common/osal_debug.c
  @brief   Debug related code.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Debug related code for Windows 32. When OSAL is compiled with nonzero OSAL_DEBUG flag, the code
  to detect program errors is included in compilation. If a programming error is detected, the
  osal_debug_error() function will be called. To find cause of a programming error, place
  a breakpoint within osal_debug error. When debugger stops to the breakpoint follow
  call stack to find the cause.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"

/* Include code from here only if debug code is to be compiled in.
 */
#if OSAL_DEBUG

/* If file name and line number are to appear in error messages.
 */
#if OSAL_DEBUG_FILE_AND_LINE


/**
****************************************************************************************************

  @brief Report a programming error.
  @anchor osal_debug_error_func

  The osal_debug_error_func()...

  Set breakpoint to this function to trap programming errors.

  @param   text Pointer to error text to log. 

  @return  None.

****************************************************************************************************
*/
void osal_debug_error_func(
    const os_char *text,
    const os_char *file,
	os_int line)
{
    os_char nbuf[22], *p;

	/* Write error message on debug console, if any.
	 */
	osal_console_write(text);

    /* Strip path from file name to keep output more readable.
     */
    p = os_strechr((os_char*)file, '/');
    if (p) file = p + 1;
    p = os_strechr((os_char*)file, '\\');
    if (p) file = p + 1;

	/* Write file name and line number.
	 */
	osal_console_write(". file: ");
	osal_console_write(file);
	osal_console_write(", line: ");
	osal_int_to_string(nbuf, sizeof(nbuf), line);
	osal_console_write(nbuf);

	/* Write terminating line feed character.
	 */
	osal_console_write("\n");
}

/**
****************************************************************************************************

  @brief Report a programming error.
  @anchor osal_debug_error_int_func

  The osal_debug_error_int_func()...

  Set breakpoint to this function to trap programming errors.

  @param   text Pointer to error text to log. 

  @return  None.

****************************************************************************************************
*/
void osal_debug_error_int_func(
    const os_char *text,
    os_long v,
    const os_char *file,
	os_int line)
{
    os_char nbuf[22], *p;

	/* Write error message on debug console, if any.
	 */
	osal_console_write(text);
	osal_int_to_string(nbuf, sizeof(nbuf), v);
	osal_console_write(nbuf);

    /* Strip path from file name to keep output more readable.
     */
    p = os_strechr((os_char*)file, '/');
    if (p) file = p + 1;
    p = os_strechr((os_char*)file, '\\');
    if (p) file = p + 1;

	/* Write file name and line number.
	 */
 	osal_console_write(". file: ");
	osal_console_write(file);
	osal_console_write(", line: ");
	osal_int_to_string(nbuf, sizeof(nbuf), line);
	osal_console_write(nbuf);

	/* Write terminating line feed character.
	 */
	osal_console_write("\n");
}


/**
****************************************************************************************************

  @brief Report programming error if cond is zero.
  @anchor osal_debug_assert_func

  The osal_debug_assert_func() calls osal_debug_error to report a programming error.

  Set breakpoint to this function to trap programming errors.

  @param   text Pointer to error text to log. 

  @return  None.

****************************************************************************************************
*/
void osal_debug_assert_func(
    os_long cond, 
	os_char *file, 
	os_int line)
{
	if (!cond)
	{
        osal_debug_error_func("Assert failed", file, line);
	}
}

/* No file name and line number in error messages. 
 */
#else

/**
****************************************************************************************************

  @brief Report a programming error.
  @anchor osal_debug_error

  The osal_debug_error()...

  Set breakpoint to this function to trap programming errors.

  @param   text Pointer to error text to log. 

  @return  None.

****************************************************************************************************
*/
void osal_debug_error(
    const os_char *text)
{
	/* Write error message on debug console, if any.
	 */
	osal_console_write(text);
	osal_console_write(".\n");
}


/**
****************************************************************************************************

  @brief Report a programming error.
  @anchor osal_debug_error

  The osal_debug_error()...

  Set breakpoint to this function to trap programming errors.

  @param   text Pointer to error text to log. 

  @return  None.

****************************************************************************************************
*/
void osal_debug_error(
    const os_char *text,
    os_long v)
{
    os_char nbuf[22];

	/* Write error message and integer argument on debug console, if any.
	 */
	osal_console_write(text);
	osal_int_to_string(nbuf, sizeof(nbuf), v);
	osal_console_write(".\n");
}

/**
****************************************************************************************************

  @brief Report programming error if cond is zero.
  @anchor osal_debug_assert_func

  The osal_debug_assert_func() calls osal_debug_error to report a programming error.


  @param   text Pointer to error text to log. 

  @return  None.

****************************************************************************************************
*/
void osal_debug_assert(
    os_long cond)
{
	if (!cond)
	{
		osal_debug_error("Assert failed");
	}
}


#endif /* OSAL_DEBUG_FILE_AND_LINE */
#endif /* OSAL_DEBUG */

