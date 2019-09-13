/**

  @file    main/common/osal_simloop.c
  @brief   Micro-controller simulation, call osal_loop repeatedly.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    12.9.2019

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_MAIN_SUPPORT

/**
****************************************************************************************************

  @brief Call osal_loop() in loop.
  @anchor osal_simulated_loop

  The osal_simulated_loop() function is used to create repeated osal_loop() function calls in PC
  environment. This function is not used in microcontroller environment, where loop is
  called by the framework.

  @param   prm Void pointer, reserved to pass context structure, etc.
  @return  None.

****************************************************************************************************
*/
void osal_simulated_loop(
    void *prm)
{
    while (!osal_loop(prm))
    {
        os_timeslice();
    }

    osal_main_cleanup(prm);
}

#endif
