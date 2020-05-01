/**

  @file    debugcode/common/osal_debug.c
  @brief   Debug related code.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Debug related code for Windows 32. When OSAL is compiled with nonzero OSAL_DEBUG flag, the code
  to detect program errors is included in compilation. If a programming error is detected, the
  osal_debug_error() function will be called. To find cause of a programming error, place
  a breakpoint within osal_debug error. When debugger stops to the breakpoint follow
  call stack to find the cause.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
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

  @brief Write file name and error number to debug console.
  @anchor osal_append_C_file_name_and_line_nr(

  The osal_append_C_file_name_and_line_nr() function writes C source file path and line number
  where error, etc. occurred to console. This is useful for findinf source of problem.

  Set breakpoint to this function to trap programming errors.

  @param   file Path to C source file.
  @param   line Line number within C source file.
  @param   new_line_at_end OS_TRUE to append new line character at end. Also path and line number are
           not appended.

  @return  None.

****************************************************************************************************
*/
static void osal_append_C_file_name_and_line_nr(
    const os_char *file,
    os_int line,
    os_boolean new_line_at_end)
{
    os_char nbuf[22], *p;

    /* Strip path from file name to keep output more readable.
     */
    p = os_strechr((os_char*)file, '/');
    if (p) file = p + 1;
    p = os_strechr((os_char*)file, '\\');
    if (p) file = p + 1;

    if (new_line_at_end)
    {
        /* Write file name and line number.
         */
        osal_console_write(". file: ");
        osal_console_write(file);
        osal_console_write(", line: ");
        osal_int_to_str(nbuf, sizeof(nbuf), line);
        osal_console_write(nbuf);

        /* Write terminating line feed character.
         */
        osal_console_write("\n");
    }
}


/**
****************************************************************************************************

  @brief Report a programming error.
  @anchor osal_debug_error_func

  The osal_debug_error_func() writes text, path to C file and line number within C file to
  console. Call this function to report errors and to generate trace, basically only for
  debug output.

  @param   text Pointer to text to log. If text starts with '~', no new line character is appended.
  @param   file Path to C source file.
  @param   line Line number within C source file.

  @return  None.

****************************************************************************************************
*/
void osal_debug_error_func(
    const os_char *text,
    const os_char *file,
    os_int line)
{
    os_boolean new_line_at_end = OS_TRUE;
    if (osal_global->quiet_mode) return;
    if (*text == '~') { new_line_at_end = OS_FALSE; text++; }

    /* Write error message on debug console, if any.
     */
    osal_console_write(text);

    /* Strip path from file name to keep output more readable.
     */
    osal_append_C_file_name_and_line_nr(file, line, new_line_at_end);
}


/**
****************************************************************************************************

  @brief Report a programming error.
  @anchor osal_debug_error_int_func

  The osal_debug_error_int_func() writes text, integer argument, path to C file and line number
  within C file to console. Call this function to report errors and to generate trace, basically
  only for debug output.

  @param   text Pointer to text to log. If text starts with '~', no new line character is appended.
  @param   v Integer number to append to text.
  @param   file Path to C source file.
  @param   line Line number within C source file.

  @return  None.

****************************************************************************************************
*/
void osal_debug_error_int_func(
    const os_char *text,
    os_long v,
    const os_char *file,
    os_int line)
{
    os_char nbuf[OSAL_NBUF_SZ];
    os_boolean new_line_at_end = OS_TRUE;
    if (osal_global->quiet_mode) return;
    if (*text == '~') { new_line_at_end = OS_FALSE; text++; }

    /* Write error message on debug console, if any.
     */
    osal_console_write(text);
    osal_int_to_str(nbuf, sizeof(nbuf), v);
    osal_console_write(nbuf);

    /* Strip path from file name to keep output more readable.
     */
    osal_append_C_file_name_and_line_nr(file, line, new_line_at_end);
}



/**
****************************************************************************************************

  @brief Report a programming error.
  @anchor osal_debug_error_str_func

  The osal_debug_error_str_func() writes text, integer argument, path to C file and line number
  within C file to console. Call this function to report errors and to generate trace, basically
  only for debug output.

  @param   text Pointer to text to log. If text starts with '~', no new line character is appended.
  @param   v String to append to text.
  @param   file Path to C source file.
  @param   line Line number within C source file.

  @return  None.

****************************************************************************************************
*/
void osal_debug_error_str_func(
    const os_char *text,
    const os_char *v,
    const os_char *file,
    os_int line)
{
    os_boolean new_line_at_end = OS_TRUE;
    if (osal_global->quiet_mode) return;
    if (*text == '~')
    {
        new_line_at_end = OS_FALSE;
        text++;
    }

    /* Write error message on debug console, if any.
     */
    osal_console_write(text);
    osal_console_write(v);

    /* Strip path from file name to keep output more readable.
     */
    osal_append_C_file_name_and_line_nr(file, line, new_line_at_end);
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
    if (osal_global->quiet_mode) return;
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
    const os_char *end_with = ".\n";
    if (osal_global->quiet_mode) return;
    if (*text == '~') { end_with = osal_str_empty; text++; }

    /* Write error message on debug console, if any.
     */
    osal_console_write(text);
    osal_console_write(end_with);
}


/**
****************************************************************************************************

  @brief Report a programming error.
  @anchor osal_debug_error_int

  The osal_debug_error()...

  Set breakpoint to this function to trap programming errors.

  @param   text Pointer to error text to log.
  @param   v Integer to append to text.

  @return  None.

****************************************************************************************************
*/
void osal_debug_error_int(
    const os_char *text,
    os_long v)
{
    const os_char *end_with = ".\n";
    os_char nbuf[22];
    if (osal_global->quiet_mode) return;
    if (*text == '~') { end_with = osal_str_empty; text++; }

    /* Write error message and integer argument on debug console, if any.
     */
    osal_console_write(text);
    osal_int_to_str(nbuf, sizeof(nbuf), v);
    osal_console_write(nbuf);
    osal_console_write(end_with);
}


/**
****************************************************************************************************

  @brief Report a programming error.
  @anchor osal_debug_error_int

  The osal_debug_error()...

  Set breakpoint to this function to trap programming errors.

  @param   text Pointer to error text to log.
  @param   v String to append to text.

  @return  None.

****************************************************************************************************
*/
void osal_debug_error_str(
    const os_char *text,
    const os_char *v)
{
    const os_char *end_with = ".\n";
    if (osal_global->quiet_mode) return;
    if (*text == '~') { end_with = osal_str_empty; text++; }

    /* Write error message and integer argument on debug console, if any.
     */
    osal_console_write(text);
    osal_console_write(v);
    osal_console_write(end_with);
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
    if (osal_global->quiet_mode) return;

    if (!cond)
    {
        osal_debug_error("Assert failed");
    }
}


#endif /* OSAL_DEBUG_FILE_AND_LINE */

#endif /* OSAL_DEBUG */

