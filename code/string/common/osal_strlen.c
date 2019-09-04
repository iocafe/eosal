/**

  @file    string/commmon/osal_strlen.c
  @brief   Get string size in bytes.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"


/**
****************************************************************************************************

  @brief Get string size in bytes.
  @anchor os_strlen

  The os_strlen() function gets size of the string in bytes, including the terminating 
  null character.

  @param   str String pointer. If the str pointer is OS_NULL, the function returns 1.

  @return  Number of bytes needed to store the string, including terminating null character.

****************************************************************************************************
*/
os_memsz os_strlen(
    const os_char *str)
{
	os_memsz count;

	count = 1;
	if (str) while (*(str++)) count++;
	return count;
}
