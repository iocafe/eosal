/**

  @file    eosal/examples/hello_world/code/hello_world_example.c
  @brief   Just make sure it builds.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This example is just a test code to test if eosal builds on target platform and can write 
  to console.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"


/**
****************************************************************************************************

  @brief Process entry point.

  The osal_main() function is OS independent entry point.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
os_int osal_main(
    os_int argc,
    os_char *argv[])
{
    osal_console_write("hello world");
    return 0;
}

