/**

  @file    eosal/extensions/process/windows/osal_windows_create_process.h
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
#include <Windows.h>


/**
****************************************************************************************************

  @brief Start a new process
  @anchor osal_create_process

  The osal_create_process() function is used to create a new child process that executes a
  specified file.

  - NULL as first argument tells CreateProcessW allows to search by PATH.
  - CREATE_NEW_CONSOLE can be omitted if we wait for process to return.

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
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    osalStatus s = OSAL_SUCCESS;
    int rval;

    os_memclear(&si, sizeof(si));
    si.dwXSize = 800;
    si.dwYSize = 600;
    si.dwXCountChars = 80;
    si.dwYCountChars = 25;
    si.dwFillAttribute = 17;
    si.fwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_MINIMIZE;

    /* Make command line by merging args */
    cmdline == ...

    /* Create child process. Notice that the first argument may look strange,
     */
    rval = CreateProcessW(file,
        cmdline, NULL, NULL, FALSE,
        CREATE_NEW_CONSOLE|NORMAL_PRIORITY_CLASS,
        NULL, NULL, &si, &pi));

    if (!rval) {
        rval = CreateProcessW(file,
            cmdline, NULL, NULL, FALSE,
            CREATE_NEW_CONSOLE|NORMAL_PRIORITY_CLASS,
            NULL, NULL, &si, &pi));
    }

    if (!rval) {
        osal_debug_error_str("Starting process failed: ", file);
        s = OSAL_STATUS_FAILED;
    }

    return s;
}

#endif
