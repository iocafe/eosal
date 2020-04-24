/**

  @file    console/common/osal_sysconsole.h
  @brief   Operating system default console IO.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  The osal_sysconsole_write() function writes text to the console or serial port designated
  for debug output, and the osal_sysconsole_read() prosesses input from system console or
  serial port.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#if OSAL_CONSOLE

/**
****************************************************************************************************

  @name Operating system's debug console functions

****************************************************************************************************
 */
/*@{*/

/* Initialize system console.
 */
void osal_sysconsole_initialize(
    void);

/* Write text to system console.
 */
void osal_sysconsole_write(
    const os_char *text);

/* Read input from system console.
 */
os_uint osal_sysconsole_read(
    void);

/*@}*/


#endif
