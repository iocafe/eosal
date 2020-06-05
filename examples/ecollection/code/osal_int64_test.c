/**

  @file    eosal/examples/ecollection/code/osal_int64_test.c
  @brief   Test 64 bit integer arithmetic on operating system.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Test 64 bit int artihmetic. Meaningfull only if compiler doesn't support 64 bit integern types.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "osal_example_collection_main.h"


/** Test data array size. To test embedded systems keep the array small because test time
    increases by square of data size.
 */
#define INT64_TEST_N_DATA 1024

/** Test data array. 64 bit integers.
 */
os_int64 int64_test_data[INT64_TEST_N_DATA];


/**
****************************************************************************************************

  @brief Process entry point.

  The osal_int64_test() function is OS independent entry point.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
osalStatus osal_int64_test(
    os_int argc,
    os_char *argv[])
{
    os_int
        v,
        i,
        j;

    os_int64
        x,
        y,
        z;

#if OSAL_SMALL_ENDIAN
#ifdef __GNUC__
    #define INT64_LARGE_DIV_TEXT
    typedef long long int64_ostype;
#else
#ifdef OSAL_WINDOWS
    #define INT64_LARGE_DIV_TEXT
    typedef __int64 int64_ostype;
#endif
#endif
#endif

#ifdef INT64_LARGE_DIV_TEXT
    int64_ostype
        xx,
        yy,
        zz;
#endif

    /* Generate test data.
     */
    v = 3;
    for (i = 0; i<INT64_TEST_N_DATA; ++i)
    {
        osal_int64_set_long(int64_test_data+i, v);
        v *= -7;
    }

    osal_int64_set_long(&z, 1000000);
    osal_int64_set_long(&x, 200000);
    osal_int64_set_long(&y, 10000);
    osal_int64_multiply(&x, &z);
    osal_int64_divide(&x, &y);

    /* Addition, subtraction and comparation with small numbers.
     */
    osal_console_write ("Addition and subtraction test... ");
    for (i = 0; i < INT64_TEST_N_DATA; i++)
    {
        osal_int64_copy(&x, int64_test_data+i);
        for (j = 0; j < INT64_TEST_N_DATA; j++)
        {
            osal_int64_copy(&z, &x);
            osal_int64_copy(&y, int64_test_data+j);

            osal_int64_add(&z, &y);

            if (osal_int64_get_long(&z) != osal_int64_get_long(&x) + osal_int64_get_long(&y))
            {
                osal_console_write ("addition failed 1\n");
                goto getout;
            }

            /* Now z and x should be different if y is nonzero.
             */
            if (!osal_int64_compare(&z, &x) && !osal_int64_is_zero(&y))
            {
                osal_console_write ("addition failed 2\n");
                goto getout;
            }

            osal_int64_subtract(&z, &y);

            /* Now z and x should be back to same.
             */
            if (osal_int64_compare(&z, &x))
            {

                osal_console_write("addition or subtraction failed\n");
                goto getout;
            }
        }
    }
    osal_console_write ("ok\n");


    /* Multiplication and division test.
     */
    osal_console_write ("Multiplication and division test... ");
    for (i = 0; i < INT64_TEST_N_DATA; i++)
    {
        osal_int64_copy(&x, int64_test_data+i);
        for (j = 0; j < INT64_TEST_N_DATA; j++)
        {
            osal_int64_copy(&z, &x);
            osal_int64_copy(&y, int64_test_data+j);

            osal_int64_multiply(&z, &y);
            osal_int64_divide(&z, &y);

            /* Now z and x should be back to same.
             */
            if (osal_int64_compare(&z, &x))
            {

                osal_console_write("multiplication or division failed\n");
                goto getout;
            }
        }
    }
    osal_console_write ("ok\n");

#ifdef INT64_LARGE_DIV_TEXT
    /* Large integer division test.
     */
    osal_console_write("Large integer division test... ");
    osal_int64_set_long(&z, 7113511);
    for (i = 0; i < INT64_TEST_N_DATA; i++)
    {
        osal_int64_multiply(int64_test_data+i, &z);
    }
    for (i = 0; i < INT64_TEST_N_DATA; i++)
    {
        osal_int64_copy(&x, int64_test_data+i);
        for (j = 0; j < INT64_TEST_N_DATA; j++)
        {
            osal_int64_copy(&z, &x);
            osal_int64_copy(&y, int64_test_data+j);

            osal_int64_divide(&z, &y);

            xx = *(int64_ostype*)&x;
            yy = *(int64_ostype*)&y;
            zz = *(int64_ostype*)&z;

            /* Now z and x should be back to same.
             */
            if (xx / yy != zz)
            {

                osal_console_write ("large division failed\n");
                goto getout;
            }
        }
    }
    osal_console_write ("ok\n");
#endif

getout:
    return OSAL_SUCCESS;
}
