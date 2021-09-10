/**

  @file    process/linux/osal_linux_create_process.h
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
#ifdef OSAL_LINUX
#if OSAL_PROCESS_SUPPORT

/// Is _GNU_SOURCE good here, it was at beginning of the file. 
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

/* This must be 1 for now. Without it program will crash in signal handling if
   it has not root privilige (otherwise works with 0 also).
 */
#define OSAL_USE_SYSCALL_TO_ELEVATE 1

#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#if OSAL_USE_SYSCALL_TO_ELEVATE
#include <syscall.h>
#endif

/* Root user and group ID is zero.
 */
#ifndef TARGET_UID
#define TARGET_UID 0
#endif
#ifndef TARGET_GID
#define TARGET_GID 0
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
  @param   exit_status Pointer where to store exit status of process if OSAL_PROCESS_WAIT
           was given. OS_NULL if not needed.
  @param   flags OSAL_PROCESS_DEFAULT just starts the process. OSAL_PROCESS_WAIT causes
           the function to return only when started process has been terminated.
           OSAL_PROCESS_ELEVATE flag = run as root.

  @return  OSAL_SUCCESS if new process was started. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus osal_create_process(
    const os_char *file,
    os_char *const argv[],
    os_int *exit_status,
    os_int flags)
{
    posix_spawnattr_t spawnattr;
    pid_t pid;
    osalStatus s;
    int rval, status;
    uid_t uid = 0;
    gid_t gid = 0;

    /* Set up PATH so that we can find system and iocom binaries.
     * "/coderoot/bin/linux" ?
     */
    static os_char *const envp[] = {
        "PATH=/usr/local/sbin:"
        "/usr/sbin:"
        "/sbin:"
        "/usr/local/bin:"
        "/usr/bin:"
        "/bin" , OS_NULL};

    /* Set zero exit status by default.
     */
    if (exit_status) {
        *exit_status = 0;
    }

    /* Switch to use root user and group. We need setuid bit for binary file set to make
       this work. It would be nice use effective user and group, we have those already set by
       setuid bit, but for dpkg we need to change real user. There could be a better way, maybe
       to tell spawn which used and group to use. But despite of reading, I did not find one.
       The syscall() allow us to set user and group just for one thread, setuid/setgid modify
       all threads of process and use signal to pass information.
     */
    if (flags & OSAL_PROCESS_ELEVATE)
    {
        uid = getuid();
        gid = getgid();

#if OSAL_USE_SYSCALL_TO_ELEVATE
        if (syscall(SYS_setresuid, (uid_t)TARGET_UID, -1, -1) != 0) {
            osal_debug_error("insufficient user privileges.");
            return OSAL_STATUS_NO_ACCESS_RIGHT;
        }

        if (syscall(SYS_setresgid, (uid_t)TARGET_GID, -1, -1) != 0) {
            osal_debug_error("insufficient group privileges.");
            return OSAL_STATUS_NO_ACCESS_RIGHT;
        }
#else
        if (setuid((uid_t)TARGET_UID) == -1) {
            osal_debug_error("insufficient user privileges.");
            return OSAL_STATUS_NO_ACCESS_RIGHT;
        }

        if (setgid((gid_t)TARGET_GID) == -1) {
            osal_debug_error("insufficient group privileges.");
            return OSAL_STATUS_NO_ACCESS_RIGHT;
        }
#endif
        osal_trace("ELEVATION SUCCESS");
    }

    /* Spawn the process. If we have separators in file name, try to look up first
       by file argument as given, then by PATH environment variable.
     */
    posix_spawnattr_init(&spawnattr);
    if (os_strchr((os_char*)file, '/')) {
        rval = posix_spawn(&pid, file, OS_NULL, &spawnattr, argv, envp);
    }
    else {
        rval = posix_spawnp(&pid, file, OS_NULL, &spawnattr, argv, envp);
    }

    /* WAS: rval = posix_spawn(&pid, file, OS_NULL, &spawnattr, argv, envp);
    if (rval) {
        rval = posix_spawnp(&pid, file, OS_NULL, &spawnattr, argv, envp);
    } */

    /* If we succeeded, we need to wait for process to exit if OSAL_PROCESS_WAIT
       flag is given. Call waitpid(-1, &rval, WNOHANG) in any case to kill zombies.
     */
    if (rval == 0) {
        s = OSAL_SUCCESS;
        if (flags & OSAL_PROCESS_WAIT) {
            if (waitpid(pid, &status, 0) != -1) {
                osal_debug_error_int("child process exited with status ", status);
                if (exit_status) {
                    *exit_status = status;
                }
            } else {
                osal_debug_error("waiting for process exit failed");
                s = OSAL_STATUS_FAILED;
            }
        }

        waitpid(-1, &rval, WNOHANG);
    }
    else {
        osal_debug_error_str("starting process failed: ", file);
        s = OSAL_STATUS_CREATE_PROCESS_FAILED;
    }

    /* Drop privileges.
     */
    if (flags & OSAL_PROCESS_ELEVATE)
    {
#if OSAL_USE_SYSCALL_TO_ELEVATE
        if (syscall(SYS_setresuid, uid, -1, -1) != 0) {
            osal_debug_error("cannot drop user privileges");
            return OSAL_STATUS_FAILED;
        }

        if (syscall(SYS_setresgid, gid, -1, -1) != 0) {
            osal_debug_error("cannot drop group privileges");
            return OSAL_STATUS_FAILED;
        }
#else
        if (setuid(uid) == -1) {
            osal_debug_error("cannot drop user privileges");
        }
        if (setgid(gid) == -1) {
            osal_debug_error("cannot drop group privileges");
        }
#endif
    }

    return s;
}

#endif
#endif
