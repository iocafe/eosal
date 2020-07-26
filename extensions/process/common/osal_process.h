/**

  @file    eosal/extensions/process/common/osal_process.h
  @brief   Start new process.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    3.7.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_PROCESS_H_
#define OSAL_PROCESS_H_
#include "eosalx.h"

#if OSAL_PROCESS_SUPPORT

/* Flags for osal_create_process()
 */
#define OSAL_PROCESS_DEFAULT 0
#define OSAL_PROCESS_WAIT 1
#define OSAL_PROCESS_ELEVATE 2

/* Start new process.
 */
osalStatus osal_create_process(
    const os_char *file,
    os_char *const argv[],
    os_int *exit_status,
    os_int flags);

#endif
#endif
