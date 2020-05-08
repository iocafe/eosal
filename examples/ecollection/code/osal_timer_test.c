/**

  @file    eosal/examples/ecollection/code/osal_timer_test.c
  @brief   Test passwords and crypto.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    7.5.2020

  Check if microcontroller timer works about precisely. The test prints message once per 10 seconds.


  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "osal_example_collection_main.h"
#include <stdio.h>


/**
****************************************************************************************************

  @brief Timer test

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.
  @return  None.

****************************************************************************************************
*/
osalStatus osal_timer_test(
    os_int argc,
    os_char *argv[])
{
    os_timer start_t = 0;
    os_int count = 0;

    while (1)
    {
        if (os_has_elapsed(&start_t, 10000))
        {
            os_get_timer(&start_t);
            osal_debug_error_int("timer hit, count=", ++count);
        }

        os_timeslice();
    }

    return OSAL_SUCCESS;
}


