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

static void osal_check_type(
    const os_char *name,
    os_memsz sz,
    os_memsz expected_sz)
{
    os_char message[128], nbuf[OSAL_NBUF_SZ];

    os_strncpy(message, name, sizeof(message));
    os_strncat(message, " size is ", sizeof(message));
    osal_int_to_str(nbuf, sizeof(nbuf), sz);
    os_strncat(message, nbuf, sizeof(message));

    if (sz == expected_sz) {
        os_strncat(message, ", ok\n", sizeof(message));
    }
    else {
        os_strncat(message, " differs from expected ", sizeof(message));
        osal_int_to_str(nbuf, sizeof(nbuf), expected_sz);
        os_strncat(message, nbuf, sizeof(message));
        os_strncat(message, ", ***** ERROR *****\n", sizeof(message));
    }

    osal_console_write(message);
}


/**
****************************************************************************************************

  @brief Test that type sizes are correctly defined in osal_defs.h

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.
  @return  None.

****************************************************************************************************
*/
osalStatus osal_type_test(
    os_int argc,
    os_char *argv[])
{
    OSAL_UNUSED(argc);
    OSAL_UNUSED(argv);

    osal_check_type("boolean", sizeof(os_boolean), 1);
    osal_check_type("char", sizeof(os_char), 1);
    osal_check_type("uchar", sizeof(os_uchar), 1);
    osal_check_type("short", sizeof(os_short), 2);
    osal_check_type("ushort", sizeof(os_ushort), 2);
    osal_check_type("int", sizeof(os_int), 4);
    osal_check_type("uint", sizeof(os_uint), 4);
    osal_check_type("int64", sizeof(os_int64), 8);
#if OSAL_LONG_IS_64_BITS
    osal_check_type("long", sizeof(os_long), 8);
    osal_check_type("ulong", sizeof(os_ulong), 8);
#else
    osal_check_type("long", sizeof(os_long), 4);
    osal_check_type("ulong", sizeof(os_ulong), 4);
#endif

    osal_check_type("float", sizeof(os_float), 4);
    osal_check_type("double", sizeof(os_double), 8);
#if OSAL_TIMER_IS_64_BITS
    osal_check_type("timer", sizeof(os_timer), 8);
#else
    osal_check_type("timer", sizeof(os_timer), 4);
#endif

    return OSAL_SUCCESS;
}


