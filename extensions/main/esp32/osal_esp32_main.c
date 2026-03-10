/**

  @file    main/linux/osal_esp32_main.c
  @brief   ESP32 process entry point function.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    10.3.2026

  This OSAL main process entry point function. Generally the operating system calls entry
  point function to start the process. Unfortunately name, arguments and character encoding
  and other detail for this entry point function varies.

  To be start a process in generic way we write osal_main() function in our application
  and then link with osal_main, etc. library which contains appropriate operating system
  dependent entry point.

  Notice that using osal_main() function to enter the process is optional, you can start the
  process in any way you like.

  ESP32 notes:
  - The app_main function takes no arguments. Dummy argument "C" is passed to application
    to application entry point.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#ifdef OSAL_ESP32
#if OSAL_MAIN_SUPPORT

/**
****************************************************************************************************

  @brief Linux process entry point.
  @anchor main

  The main() function is used generally as entry point. This function just removes extra '\r'
  character, if one is found.

  @param   argc Number of command line arguments. First argument is name of the executable.
  @param   argv Array of string pointers, one for each command line argument plus first
           one for the executable name, UTF16 encoded.

  @return  Integer return value to caller.

****************************************************************************************************
*/
int eosal_entry(
    int argc,
    char **argv)
{
    static os_char *arglist[1] = {"C"};

    /* Initialize operating system abstraction layer.
     */
    osal_initialize(OSAL_INIT_DEFAULT);

    /* Call the application entry point function
     */
    osal_main(1, arglist);

    /* Shut down operating system abstraction layer.
     */
    osal_shutdown();

    return 0;
}

#endif
#endif
