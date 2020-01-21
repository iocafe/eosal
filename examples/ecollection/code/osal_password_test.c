/**

  @file    eosal/examples/ecollection/code/osal_password_test.c
  @brief   Test passwords and crypto.
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

  @brief Password handling test

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.
  @return  None.

****************************************************************************************************
*/
osalStatus osal_password_test(
    os_int argc,
    os_char *argv[])
{
    os_char password[OSAL_HASH_STR_SZ], hashed[OSAL_HASH_STR_SZ];
    os_short i;

    for (i = 0; i<10; ++i)
    {
        osal_make_random_password(password, sizeof(password));
        printf("random password = %s\n", password);

        osal_hash_password(hashed, sizeof(hashed), password);
        printf("hashed password = %s\n", hashed);
    }

    return OSAL_SUCCESS;
}


