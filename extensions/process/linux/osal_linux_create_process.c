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
#define _GNU_SOURCE
#include "eosalx.h"
#if OSAL_PROCESS_SUPPORT

#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#ifndef TARGET_UID
#define TARGET_UID 0
#endif

#ifndef TARGET_GID
#define TARGET_GID 0
#endif

#ifndef UID_MIN
#define UID_MIN 500
#endif

#ifndef GID_MIN
#define GID_MIN 500
#endif

/**
****************************************************************************************************

  @brief Start a new process
  @anchor osal_create_process

  The osal_create_process() function is used to create a new child process that executes a
  specified file.

  - waitpid(-1, &rval, WNOHANG) is called to kill zombies.

  @param   file Name or path to file to execute.
  @param   argv Array of command line arguments. OS_NULL pointer terminates the array.
  @param   flags OSAL_PROCESS_DEFAULT just starts the process. OSAL_PROCESS_WAIT causes
           the function to return only when started process has been terminated.

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
    int rval, status;
    uid_t euid; /* Real, Effective, Saved user ID */
    gid_t egid; /* Real, Effective, Saved group ID */

    static os_char *const envp[] = {"PATH=/usr/local/sbin:/usr/sbin:/sbin:/usr/local/bin:/usr/bin:/bin:/coderoot/bin/linux", OS_NULL};

    if (flags & OSAL_PROCESS_ELEVATE)
    {
        euid = getuid();
        egid = getgid();

         /* Switch to target user. setuid bit handles this, but doing it again does no harm. */
         if (setuid((uid_t)TARGET_UID) == -1) {
             osal_debug_error("Insufficient user privileges.");
             return OSAL_STATUS_FAILED;
         }

         /* Switch to target group. setgid bit handles this, but doing it again does no harm.
          * If TARGET_UID == 0, we need no setgid bit, as root has the privilege. */
         if (setgid((gid_t)TARGET_GID) == -1) {
             osal_debug_error("Insufficient group privileges.");
             return OSAL_STATUS_FAILED;
         }
        osal_debug_error("ELEVATION SUCCESS");
    }

    posix_spawnattr_init(&spawnattr);
    rval = posix_spawn(&pid, file, OS_NULL, &spawnattr, argv, envp);
    if (rval) {
        rval = posix_spawnp(&pid, file, OS_NULL, &spawnattr, argv, envp);
    }

    if (rval == 0) {
        s = OSAL_SUCCESS;
        if (flags & OSAL_PROCESS_WAIT) {
            if (waitpid(pid, &status, 0) != -1) {
                osal_debug_error_int("child process exited with status ", status);
            } else {
                osal_debug_error("waiting for process exit failed");
                s = OSAL_STATUS_FAILED;
            }
        }

        waitpid(-1, &rval, WNOHANG);
    }
    else {
        osal_debug_error_str("Starting process failed: ", file);
        s = OSAL_STATUS_FAILED;
    }

    /* Drop privileges. */
    if (flags & OSAL_PROCESS_ELEVATE)
    {
        if (setuid(euid) == -1) {
            osal_debug_error("Cannot drop user privileges");
        }
        if (setgid(egid) == -1) {
            osal_debug_error("Cannot drop group privileges");
        }
    }

    return s;
}

#endif
