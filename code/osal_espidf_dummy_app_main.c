/**

  @file    osal_espidf_dummy_app_main.c
  @brief   Dummy file to allow testing eosal ESPIDF build configuration.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    4.9.2021

  Dummy app_main() to compile eosal separately.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#ifdef OSAL_ESP32

#ifdef OSAL_DUMMY_ESPIDF_APP_MAIN
void app_main(void)
{
    osal_initialize(OSAL_INIT_DEFAULT);
}
#endif

#endif
