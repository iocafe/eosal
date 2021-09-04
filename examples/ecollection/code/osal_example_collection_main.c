/**

  @file    eosal/examples/ecollection/code/osal_example_collection_main.c
  @brief   Example code about threads.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Main function for set of example. Multiple simple examples/tests have been packed in this
  project to avoid creating many projets.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#include "osal_example_collection_main.h"

/* If needed for the operating system, EOSAL_C_MAIN macro generates the actual C main() function.
 */
EOSAL_C_MAIN

/**
****************************************************************************************************

  @brief Process entry point.

  The osal_main() function is OS independent entry point.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
osalStatus osal_main(
    os_int argc,
    os_char *argv[])
{
    // osal_sleep(7000);
    osal_detached_thread_example();
    // osal_attached_thread_example();
    // return osal_int64_test(argc, argv);
    // return osal_intser_test(argc, argv);
    // return osal_json_compress_test(argc, argv);
    // return osal_rand_test(argc, argv);
    // return osal_float_int_conv_test(argc, argv);
    // return osal_password_test(argc, argv);
    // return osal_persistent_test(argc, argv);
    // return osal_timer_test(argc, argv);
    // return osal_type_test(argc, argv);

    return OSAL_SUCCESS;
}


/*  Empty function implementation needed to build for microcontroller.
 */
osalStatus osal_loop(
    void *app_context)
{
    OSAL_UNUSED(app_context);
    return OSAL_SUCCESS;
}

/*  Empty function implementation needed to build for microcontroller.
 */
void osal_main_cleanup(
    void *app_context)
{
    OSAL_UNUSED(app_context);
}
