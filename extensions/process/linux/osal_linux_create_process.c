/**

  @file    eosal/extensions/process/linux/osal_linux_create_process.h
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
#include "eosalx.h"
#if OSAL_PROCESS_SUPPORT

#define _GNU_SOURCE
#include <spawn.h>
#include <sys/wait.h>

/**
****************************************************************************************************

  @brief Start a new process
  @anchor osal_create_process

  The osal_create_process() function is used to create a new child process that executes a
  specified file.

  - waitpid() is called to kill zombies.

  @param   file Name or path to file to execute.
  @param   argv Array of command line arguments. OS_NULL pointer terminates the array.
  @param   flags Reserved for future, set zero for now.
  @return  OSAL_SUCCESS if new process was started. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus osal_create_process(
    const os_char *file,
    os_char *const argv[],
    os_int flags)
{
    posix_spawnattr_t spawnattr;
    pid_t pid;
    osalStatus s;
    int rval;

    posix_spawnattr_init(&spawnattr);
    rval = posix_spawn(&pid, file, NULL, &spawnattr, argv, NULL);
    if (rval) {
        rval = posix_spawnp(&pid, file, NULL, &spawnattr, argv, NULL);
    }

    if (rval == 0) {
        waitpid(-1, &rval, WNOHANG);
        s = OSAL_SUCCESS;
    }
    else {
        osal_debug_error_str("Starting process failed: ", file);
        s = OSAL_STATUS_FAILED;
    }

    return s;
}

#endif
