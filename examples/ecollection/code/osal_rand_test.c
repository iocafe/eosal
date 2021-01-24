/**

  @file    eosal/examples/ecollection/code/osal_intset_test.c
  @brief   Test integer serialization.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020


  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "osal_example_collection_main.h"
#include <stdio.h>

static void osal_rand_test_range(
    os_long min_value,
    os_long max_value,
    os_int n,
    os_boolean print_numbers);


/**
****************************************************************************************************

  @brief Process entry point.

  The osal_intser_test() function is OS independent entry point.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
osalStatus osal_rand_test(
    os_int argc,
    os_char *argv[])
{
    OSAL_UNUSED(argc);
    OSAL_UNUSED(argv);

    /* For ESP32 it is recommended to initialize WiFi or blue tooth to get hardware
       random numbers. In practice I got random numbers even without this, maybe
       the note relates to some older esp-idf version.
     */
#if OSAL_SOCKET_SUPPORT
    osal_socket_initialize(OS_NULL, 0, OS_NULL, 0);
#endif

    osal_rand_test_range(-1000, 1000, 100, OS_TRUE);
    osal_rand_test_range(-10000000000, 10000000000, 100, OS_TRUE);
    osal_rand_test_range(0, 0, 100, OS_TRUE);
    osal_rand_test_range(-1000, -990, 100, OS_TRUE);
    osal_rand_test_range(-10000000000, 10000000000, 10000, OS_FALSE);

    return OSAL_SUCCESS;
}


static void osal_rand_test_range(
    os_long min_value,
    os_long max_value,
    os_int n,
    os_boolean print_numbers)
{
    os_int i;
    os_long x;

    for (i = 0; i < n; i++)
    {
        x = osal_rand(min_value, max_value);
        if (min_value != max_value) if (x < min_value || x > max_value)
        {
            printf ("\nRand failed min=%ld max=%ld value=%ld ************************\n",
                (long)min_value,(long) max_value, (long)x);
            return;
        }

        if (print_numbers)
        {
            printf ("%ld ", (long)x);
        }
    }
}
