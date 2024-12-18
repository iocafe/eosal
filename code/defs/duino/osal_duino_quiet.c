/**

  @file    defs/duino/osal_duino_quiet.c
  @brief   Global OSAL state.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    24.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#ifdef OSAL_ARDUINO

/**
****************************************************************************************************

  @brief Silence debug, etc. prints.
  @anchor osal_quiet

  The osal_quiet() enables (or disables) quiet mode. Quiet mode allows user to use console
  for example for setting wifi network name and password.

  @param   enable OS_TRUE to enable quiet mode, OS_FALSE to allow debug prints.
  @return  Previous value: OS_TRUE if quiet mode was enabed before this call, OS_FALS if not.

****************************************************************************************************
*/
os_boolean osal_quiet(
    os_boolean enable)
{
    os_boolean prev_value;
    prev_value = osal_global->quiet_mode;
    osal_global->quiet_mode = enable;
    return prev_value;
}

#endif
