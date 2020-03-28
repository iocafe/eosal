/**

  @file    console/esp/osal_sysconsole.c
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
#include "eosal.h"
#if OSAL_CONSOLE

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"


/**
****************************************************************************************************

  @brief Write text to system console.
  @anchor osal_sysconsole_write

  The osal_sysconsole_write() function writes a string to process'es default console, if any.

  @param   text Pointer to string to write.

  @return  Pointer to the allocated memory block, or OS_NULL if the function failed (out of
           memory).

****************************************************************************************************
*/
void osal_sysconsole_write(
	const os_char *text)
{
    ESP_LOGI(eosal_mod, "%s", text);
}


/**
****************************************************************************************************

  @brief Read character from system console.
  @anchor osal_sysconsole_run

  The osal_sysconsole_read() function reads the input from system console. If there are any
  input, the callbacks monitoring the input from this console will get called.

  @return  UTF32 character or 0 if none.

****************************************************************************************************
*/
os_uint osal_sysconsole_read(
    void)
{
    return 0;
}


#endif