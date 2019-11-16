/**

  @file    eosal/examples/ecollection/code/osal_float_int_conv_test.c
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
#include <math.h>

/* Forward referred static functions.
 */
static osalStatus osal_test_double_value(
    os_double x);

static osalStatus osal_test_float_value(
    os_float x);


/**
****************************************************************************************************

  @brief Process entry point.

  The osal_intser_test() function is OS independent entry point.
   
  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
osalStatus osal_float_int_conv_test(
    os_int argc,
    os_char *argv[])
{
    osalStatus s;
    os_long x, m;
    os_int i;
    os_short e;
    os_float f;

    s = osal_test_double_value(0);
    if (s) goto failed;
    s = osal_test_float_value(0);
    if (s) goto failed;

    osal_double2ints(2.1, &m, &e);
    osal_ints2float(&f, m, e);
    if (fabs(f - 2.1) > 1.0e-5) 
    {
        printf ("FAILED, osal_double2ints -> osal_float2ints\n");
    }

    for (i = 0; i<500000; i++)
    {
        x = osal_rand(-5000, 5000);
        s = osal_test_double_value(x * 0.7);
        s = osal_test_float_value((os_float)(x * 0.7));
        if (s) goto failed;
    }

    printf ("Success\n");
    return OSAL_SUCCESS;

failed:
    return s;
}

static osalStatus osal_test_double_value(
    os_double x)
{
    os_double y;
    os_long m;
    os_short e;

    osal_double2ints(x, &m, &e);
    osal_ints2double(&y, m, e);

    if (x != y)
    {
        printf ("FAILED, osal_double2ints(%f) -> ,\n"
            "    osal_ints2double(m=%lld, e=%lld) = %f\n", x,
            (long long)m, (long long)e, y);
        goto failed;
    }

    return OSAL_SUCCESS;

failed:
    return OSAL_STATUS_FAILED;
}


static osalStatus osal_test_float_value(
    os_float x)
{
    os_float y;
    os_long m;
    os_short e;

    osal_float2ints(x, &m, &e);
    osal_ints2float(&y, m, e);

    if (x != y)
    {
        printf ("FAILED, osal_float2ints(%f) -> ,\n"
            "    osal_ints2float(m=%lld, e=%lld) = %f\n",
            (double)x, (long long)m, (long long)e, (double)y);
        goto failed;
    }

    return OSAL_SUCCESS;

failed:
    return OSAL_STATUS_FAILED;
}
