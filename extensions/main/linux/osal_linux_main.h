/**

  @file    main/linux/osal_linux_main.h
  @brief   Linux process entry point function.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  This OSAL main process entry point header file. Generally the operating system calls entry
  point function to start the process. Unfortunately name, arguments and character encoding
  and other detail for this entry point function varies.

  To be start a process in generic way we write osal_main() function in our application
  and then link with osal_main, etc. library which contains appropriate operating system
  dependent entry point, converts the arguments to UTF8 and passes these on to osal_main()
  Use macro EOSAL_C_MAIN in application to generate actual C main() function code.

  Notice that using osal_main() function to enter the process is optional, you can start the
  process in any way you like.

  Linux notes:
  - On linux terminating '\r' on command line is removed. This is typically caused by
    editing scripts on windows.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_LINUX_MAIN_H_
#define OSAL_LINUX_MAIN_H_
#include "eosalx.h"

/* Prototype of operating system specific entry point code/
 */
int eosal_entry(
    int argc,
    char **argv);

/* Macro to generate C main() function code.
 */
#define EOSAL_C_MAIN  \
    OSAL_C_HEADER_BEGINS \
    int main(int c, char **v) {return eosal_entry((c), (v)); } \
    OSAL_C_HEADER_ENDS

#endif
