/**

  @file    eosal/examples/ecollection/code/osal_persistent_test.c
  @brief   Test persistent storage
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.3.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "osal_example_collection_main.h"
#if OSAL_PERSISTENT_SUPPORT

#define CONTENTENT_X_STR_SZ 37
//#define CONTENTENT_Y_STR_SZ 716
#define CONTENTENT_Y_STR_SZ 100

typedef struct
{
    os_char str[CONTENTENT_X_STR_SZ];
    os_int i;
}
ContentX;

typedef struct
{
    os_char str[CONTENTENT_Y_STR_SZ];
    os_int i;
}
ContentY;

/* Some test texts.
 */
const os_char test_str_1[] = "Eat cheese and salad sandwitch.";
const os_char test_str_2[] = "Do you remember president Nixon? Do you remember bills you have to pay?";

/* Prototyped for forward referred functions.
 */
static osalStatus save_x(
    os_int block_nr,
    os_int i);

static osalStatus load_x(
    os_int block_nr,
    os_int i);

static osalStatus save_y(
    os_int block_nr,
    os_int i);

static osalStatus load_y(
    os_int block_nr,
    os_int i);


/**
****************************************************************************************************

  @brief Test persistent storage

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.
  @return  None.

    OS_PBNR_CUST_A = 12,
    OS_PBNR_CUST_B = 13,
    OS_PBNR_CUST_C = 14,


****************************************************************************************************
*/
osalStatus osal_persistent_test(
    os_int argc,
    os_char *argv[])
{
    int s = 0;
    os_int i;
    OSAL_UNUSED(argc);
    OSAL_UNUSED(argv);

    /* Initialize persistent storage
     */
    os_persistent_initialze(OS_NULL);

    /* Save and load block in micellenous order to test it
     */
    for (i = 0; i<10; ++i)
    {
        s |= save_x(OS_PBNR_CUST_A, i);
        s |= load_x(OS_PBNR_CUST_A, i);
        s |= save_y(OS_PBNR_CUST_B, i);
        s |= load_x(OS_PBNR_CUST_A, i);
        s |= load_y(OS_PBNR_CUST_B, i);

        s |= save_y(OS_PBNR_CUST_C, i+5);
        s |= load_y(OS_PBNR_CUST_C, i+5);
        s |= save_y(OS_PBNR_CUST_A, i+2);
        s |= load_y(OS_PBNR_CUST_B, i);
    }

    if (!s)
    {
        osal_console_write("All good\n");
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************
  Save content X to persistent storage
****************************************************************************************************
*/
static osalStatus save_x(
    os_int block_nr,
    os_int i)
{
    ContentX x;
    osalStatus s;

    os_memclear(&x, sizeof(x));
    os_strncpy(x.str, test_str_1, CONTENTENT_X_STR_SZ);
    x.i = i;

    s = os_save_persistent(block_nr, (os_char*)&x, sizeof(x), OS_FALSE);
    if (s) {
        osal_console_write("Writing X to persistent block failed\n");
    }
    return s;
}


/**
****************************************************************************************************
  Load content X to from persistent storage
****************************************************************************************************
*/
static osalStatus load_x(
    os_int block_nr,
    os_int i)
{
    ContentX x;
    osalStatus s;
    os_char nbuf[OSAL_NBUF_SZ];

    os_memclear(&x, sizeof(x));

    s = os_load_persistent(block_nr, (os_char*)&x, sizeof(x));
    if (s) {
        osal_console_write("Loading X from persistent block ");
        osal_int_to_str(nbuf, sizeof (nbuf), block_nr);
        osal_console_write(nbuf);
        osal_console_write(" failed. ");
    }

    if (i != x.i && !OSAL_IS_ERROR(s)) {
        osal_console_write("Content mismatch ");
        s = OSAL_STATUS_FAILED;
    }

    osal_console_write(x.str);
    osal_console_write("\n");

    return s;
}


/**
****************************************************************************************************
  Save content Y to persistent storage
****************************************************************************************************
*/
static osalStatus save_y(
    os_int block_nr,
    os_int i)
{
    ContentY y;
    osalStatus s;

    os_memclear(&y, sizeof(y));
    os_strncpy(y.str, test_str_2, CONTENTENT_Y_STR_SZ);
    y.i = i;

    s = os_save_persistent(block_nr, (os_char*)&y, sizeof(y), OS_FALSE);
    if (s) {
        osal_console_write("Writing Y to persistent block failed\n");
    }
    return s;
}


/**
****************************************************************************************************
  Load content Y to from persistent storage
****************************************************************************************************
*/
static osalStatus load_y(
    os_int block_nr,
    os_int i)
{
    ContentY y;
    osalStatus s;
    os_char nbuf[OSAL_NBUF_SZ];

    os_memclear(&y, sizeof(y));

    s = os_load_persistent(block_nr, (os_char*)&y, sizeof(y));
    if (s) {
        osal_console_write("Loading Y from persistent block ");
        osal_int_to_str(nbuf, sizeof (nbuf), block_nr);
        osal_console_write(nbuf);
        osal_console_write(" failed. ");
    }

    if (i != y.i && !OSAL_IS_ERROR(s)) {
        osal_console_write("Content mismatch ");
        s = OSAL_STATUS_FAILED;
    }

    osal_console_write(y.str);
    osal_console_write("\n");

    return s;
}


#endif
