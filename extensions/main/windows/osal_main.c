/**

  @file    main/windows/osal_main.c
  @brief   Windows specific process entry point function.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This OSAL main process entry point header file. Generally the operating system calls entry
  point function to start the process. Unfortunately name, arguments and character encoding
  for this entry point function varies. For example Windows uses main, wmain, WinMain and
  wWinMain. Besides many tool libraries define their own entry point functions.

  To be start a process in generic way we write osal_main() function in our application
  and then link with osal_main, etc. library which contains appropriate operating system
  dependent entry point, converts the arguments to UTF8 and passes these on to osal_main()

  Notice that using osal_main() function to enter the process is optional, you can start the
  process in any way you like.

  Windows notes:
  - An application using /SUBSYSTEM:CONSOLE calls wmain, set wmainCRTStartup linker option
    and link with osal_maind.lib (debug) or osal_main.lib (release).
  - An application using /SUBSYSTEM:WINDOWS; calls wWinMain, which must be defined with
    __stdcall, set wWinMainCRTStartup as entry point. Link with ?

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_MAIN_SUPPORT

#include <windows.h>


/**
****************************************************************************************************

  @brief Windows console subsystem Unicode entry point.
  @anchor wmain

  The wmain() function is Windows /SUBSYSTEM:CONSOLE Unicode compilation entry point name.

  @param   argc Number of command line arguments. First argument is name of the executable.
  @param   argv Array of string pointers, one for each command line argument plus first
           one for the executable name, UTF16 encoded.

  @return  Integer return value to caller.

****************************************************************************************************
*/
int wmain(
    int argc,
    wchar_t *argv[])
{
    os_char
        **utf8_argv;

    os_int
        i;

    int
        rval;

    /* Initialize operating system abstraction layer.
     */
    osal_initialize(OSAL_INIT_DEFAULT);

    /* Set normal thread priority.
     */
#if OSAL_MULTITHREAD_SUPPORT
    osal_thread_set_priority(OSAL_THREAD_PRIORITY_NORMAL);
#endif

    /* Convert command line arguments from UTF16 to UTF8. This will be cleaned up when
       osal_shutdown runs at exit, and all memory will be released.
     */
    utf8_argv = (os_char**)os_malloc(argc*sizeof(os_char*), OS_NULL);
    for (i = 0; i < argc; i++)
    {
        utf8_argv[i] = osal_str_utf16_to_utf8_malloc(argv[i], OS_NULL);
    }

    /* Call OS independent process entry point.
     */
    rval = osal_main(argc, utf8_argv);

    /* Shut down operating system abstraction layer.
     */
    osal_shutdown();
    
    return rval;
}


/**
****************************************************************************************************

  @brief Windows console subsystem MBCS entry point.
  @anchor main

  The main() function is Windows /SUBSYSTEM:CONSOLE MBCS compilation entry point name.

  MinGW doesn't support wmain() directly, thus this intermediate entry point function is
  always needed when compiling with MinGW.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument plus.

  @return  Integer return value to caller.

****************************************************************************************************
*/
int main(
    int a_argc,
    char **a_argv)
{
    wchar_t **argv;
    int argc;

    /* Get comman line arguments as UTF16
     */
    argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    /* Call the Unicode entry point.
     */
    return wmain(argc, argv);
}

#endif
