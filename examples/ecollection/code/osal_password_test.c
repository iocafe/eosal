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
    os_char password[OSAL_SECRET_STR_SZ], hashed[OSAL_SECRET_STR_SZ];
    os_short i;
    OSAL_UNUSED(argc);
    OSAL_UNUSED(argv);

    for (i = 0; i<10; ++i)
    {
        osal_make_random_secret();
        osal_get_password(password, sizeof(password));

        printf("random password = %s\n", password);

        osal_hash_password(hashed, password, sizeof(hashed));
        printf("hashed password = %s\n", hashed);

        /* osal_forget_secret(); */
    }

    return OSAL_SUCCESS;
}


