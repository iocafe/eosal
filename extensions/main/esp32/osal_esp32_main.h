/**

  @file    main/linux/osal_esp32_main.h
  @brief   ESP32 process entry point.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    10.3.2026

  This OSAL main process entry point header file.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_ESP32_MAIN_H_
#define OSAL_ESP32_MAIN_H_
#include "eosalx.h"
#ifdef OSAL_ESP32

/* Prototype of operating system specific entry point code/
 */
int eosal_entry(
    int argc,
    char **argv);

/* Macro to generate C main() function code.
 */
#define EOSAL_C_MAIN  \
    OSAL_C_HEADER_BEGINS \
    void app_main(void) {eosal_entry(0, OS_NULL); } \
    OSAL_C_HEADER_ENDS

#endif
#endif
