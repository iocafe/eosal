/**

  @file    initialize/esp32/osal_esp32_dummy_app_main.c
  @brief   Dummy ESP32 app_main() function.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    3.9.2021

  The dummy app_main() functions enables building library as executable.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#ifdef OSAL_ESP32
#ifdef OSAL_DUMMY_ESP32_APP_MAIN

/**
****************************************************************************************************

  @brief Dummy main function to allow testing eosal ESP32 build configuration.
  @anchor app_main

  Dummy app_main() function allows compiling eosal separately as application (which does
  nothing) and allows to test ESPIDF/platformio build.

****************************************************************************************************
*/
extern "C" void app_main(void)
{
    osal_initialize(OSAL_INIT_DEFAULT);
}

#endif
#endif

