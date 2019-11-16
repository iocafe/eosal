/**

  @file    eosal/examples/ecollection/code/osal_intset_test.c
  @brief   Test integer serilization.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    25.10.2019


  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "osal_example_collection_main.h"
#include <stdio.h>

/* Forward referred static functions.
 */
static osalStatus osal_intser_test_value(
    os_long x);


/**
****************************************************************************************************

  @brief Process entry point.

  The osal_intser_test() function is OS independent entry point.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
osalStatus osal_intser_test(
    os_int argc,
    os_char *argv[])
{
    osalStatus s;
    os_long x;
    os_int i;

    s = osal_intser_test_value(3687);
    if (s) goto failed;

    s = osal_intser_test_value(-5);
    if (s) goto failed;

    s = osal_intser_test_value(9);
    if (s) goto failed;

    s = osal_intser_test_value(0x7FFFFFFFFFFFFFFF);
    if (s) goto failed;

    s = osal_intser_test_value(0x8000000000000000);
    if (s) goto failed;

    for (i = 0; i<500000; i++)
    {
        x = osal_rand(-5000, 5000);
        s = osal_intser_test_value(x);
        if (s) goto failed;
    }

    for (i = 0; i<500000; i++)
    {
        x = osal_rand(0x8000000000000000, 0x7FFFFFFFFFFFFFFF);
        s = osal_intser_test_value(x);
        if (s) goto failed;
    }

    printf ("Success\n");
    return OSAL_SUCCESS;

failed:
    return s;
}

static osalStatus osal_intser_test_value(
    os_long x)
{
    os_char buf[OSAL_INTSER_BUF_SZ];
    os_int bytes, bytes2, i;
    os_long y;
    os_uchar unsignedc;

    os_memclear(buf, sizeof(buf));
    bytes = osal_intser_writer(buf, x);
    if (bytes < 1 || bytes >= OSAL_INTSER_BUF_SZ)
    {
        printf ("FAILED, osal_intser_writer(%lld) returned errornous number of bytes\n", (long long)x);
        goto failed;
    }

    bytes2 = osal_intser_reader(buf, &y);
    if (x != y)
    {
        printf ("FAILED, osal_intser_writer(buf, %lld),\n"
            "    osal_intser_reader(buf, &y), y = %lld\n", (long long)x, (long long)y);
        goto failed;
    }

    if (bytes != bytes)
    {
        printf ("FAILED, number of bytes mismach osal_intser_writer(buf, %lld) returned %d,\n"
            "    osal_intser_reader(buf, &y), returned = %d\n", (long long)x, (int)bytes, (int)bytes2);
        goto failed;
    }

    return OSAL_SUCCESS;

failed:
    printf ("    ");
    for (i = 0; i<bytes; i++)
    {
        unsignedc = buf[i];
        printf ("%02x ", (unsigned)unsignedc);
    }
    printf ("\n");
    return OSAL_STATUS_FAILED;
}
