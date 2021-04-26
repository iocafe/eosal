/**

  @file    main/common/osal_main.h
  @brief   Calling generic entry point.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  This OSAL main process entry point header file. Generally the operating system calls entry
  point function to start the process. Unfortunately name, arguments and character encoding
  for this entry point function varies. For example Windows uses main, wmain, WinMain and
  wWinMain. Besides many tool libraries define their own entry point functions.

  To be start a process in generic way we write osal_main() function in our application
  and then link with osal_main, etc. library which contains appropriate operating system
  dependent entry point, converts the arguments to UTF8 and passes these on to osal_main()

  Notice that using osal_main() function to enter the process is optional, you can start the
  process in any way you like.

  Windows notes :
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
#ifndef OSAL_MAIN_H_
#define OSAL_MAIN_H_
#include "eosalx.h"

/* If C++ compilation, all functions, etc. from this point on in this header file are
   plain C and must be left undecorated.
 */
OSAL_C_HEADER_BEGINS

/* Context pointer is given to osal_simulated_loop() is saved here.
 */
extern void *osal_application_context;

/* Prototype for application's entry point function. We declare this prototype, even without
 * OSAL_MAIN_SUPPORT define set.
 */
osalStatus osal_main(
    os_int argc,
    os_char *argv[]);

/* Prototype for application defined loop function. This is implemented for micro-controller
 * environment to process single thread model loop calls.
 */
osalStatus osal_loop(
    void *app_context);

/* Prototype for application defined cleanup function to release resources
   allocated by osal_main().
 */
void osal_main_cleanup(
    void *app_context);

/* The osal_simulated_loop() function is used to create repeated osal_loop function calls in PC
   This function only saves context pointer when run in microcontroller environment,
   which can be used for calling loop function from the framework.
 */
void osal_simulated_loop(
    void *app_context);

/* If we are not running in microcontroller, we may want to allow setting device number
   from command line, like "-n=7".
 */
#if OSAL_MAIN_SUPPORT
    os_int osal_command_line_device_nr(
        os_int device_nr,
        os_int argc,
        os_char *argv[]);
#else
    #define osal_command_line_device_nr(n,c,v) (n)
#endif


/* If C++ compilation, end the undecorated code.
 */
OSAL_C_HEADER_ENDS

#endif
