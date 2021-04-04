/**

  @file    eosal/examples/ecollection/code/osal_example_collection_main.h
  @brief   Test code/example collection.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"

osalStatus osal_int64_test(
    os_int argc,
    os_char *argv[]);

osalStatus osal_intser_test(
    os_int argc,
    os_char *argv[]);

osalStatus osal_float_int_conv_test(
    os_int argc,
    os_char *argv[]);

osalStatus osal_json_compress_test(
    os_int argc,
    os_char *argv[]);

osalStatus osal_threads_example_main(
    os_int argc,
    os_char *argv[]);

void osal_attached_thread_example(void);

osalStatus osal_rand_test(
    os_int argc,
    os_char *argv[]);

osalStatus osal_password_test(
    os_int argc,
    os_char *argv[]);

osalStatus osal_persistent_test(
    os_int argc,
    os_char *argv[]);

osalStatus osal_timer_test(
    os_int argc,
    os_char *argv[]);

osalStatus osal_type_test(
    os_int argc,
    os_char *argv[]);
