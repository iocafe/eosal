/**

  @file    main/windows/osal_windows_main.c
  @brief   Windows specific process entry point function.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    14.8.2020

  This OSAL main process entry point header file. Generally the operating system calls entry
  point function to start the process. Unfortunately name, arguments and character encoding
  for this entry point function varies. For example Windows uses main, wmain, WinMain and
  wWinMain. Besides many tool libraries define their own entry point functions.

  To be start a process in generic way we write osal_main() function in our application
  and then link with osal_main, etc. library which contains appropriate operating system
  dependent entry point, converts the arguments to UTF8 and passes these on to osal_main()
  Use macro EOSAL_C_MAIN in application to generate actual C main() function code.

  Notice that using osal_main() function to enter the process is optional, you can start the
  process in any way you like.

  Windows notes:
  - An application using /SUBSYSTEM:CONSOLE calls wmain, set wmainCRTStartup linker option
    and link with osal_maind.lib (debug) or osal_main.lib (release).
  - An application using /SUBSYSTEM:WINDOWS; calls wWinMain, which must be defined with
    __stdcall, set wWinMainCRTStartup as entry point. Link with ?

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_WINDOWS_MAIN_H_
#define OSAL_WINDOWS_MAIN_H_
#include "eosalx.h"

/* Prototype of operating system specific entry point code, UTF16.
 */
int eosal_entry_w(
    int argc,
    wchar_t *argv[]);

/* Prototype of operating system specific entry point code, MBCS.
 */
int eosal_entry_s(
    int a_argc,
    char **a_argv);

/* Macro to generate C main() function code.
 */
#define EOSAL_C_MAIN  \
    OSAL_C_HEADER_BEGINS \
    int wmain(int c, wchar_t *v[]) {return eosal_entry_w((c), (v)); } \
    int main(int c, char **v) {return eosal_entry_s((c), (v)); } \
    OSAL_C_HEADER_ENDS

#endif
