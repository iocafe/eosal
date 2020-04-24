/**

  @file    console/common/osal_console.h
  @brief   Application console IO.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  This header file contains functions prototypes for application's console IO.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/

/**
****************************************************************************************************
  Some useful key codes
****************************************************************************************************
*/
#define OSAL_CONSOLE_ESC 27
#define OSAL_CONSOLE_ENTER '\n'
#define OSAL_CONSOLE_BACKSPACE 127

/**
****************************************************************************************************

  @name Application's Console Functions

  The osal_console_write() function writes a string to console or reads a character from
  console.

****************************************************************************************************
 */
/*@{*/

#if OSAL_CONSOLE

    /* Initialize console.
     */
    #define osal_console_initialize() osal_sysconsole_initialize()

    /* Write text to console.
     */
    #define osal_console_write(t) osal_sysconsole_write(t)

    /* Read an UTF32 character from console. 0 = no char.
     */
    #define osal_console_read() osal_sysconsole_read()

#else

    #define osal_console_initialize()
    #define osal_console_write(t)
    #define osal_console_read() 0

#endif

