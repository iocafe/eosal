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
  @param   exit_status Pointer where to store exit status of process if OSAL_PROCESS_WAIT
           was given. OS_NULL if not needed.
  @param   flags OSAL_PROCESS_DEFAULT just starts the process. OSAL_PROCESS_WAIT causes
           the function to return only when started process has been terminated.
  @return  OSAL_SUCCESS if new process was started. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus osal_create_process(
    const os_char *file,
    os_char *const argv[],
    os_int *exit_status,
    os_int flags)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    const os_char *a;
    os_char *cmdline;
    osalStream cmdline_buf;
    os_memsz n_written, cmdline_bytes;
    os_ushort *file_utf16, *cmdline_utf16;
    os_memsz file_utf16_sz, cmdline_utf16_sz;
    osalStatus s = OSAL_SUCCESS;
    os_int pos;
    int rval;

    /* Set zero exit status by default.
     */
    if (exit_status) {
        *exit_status = 0;
    }

    os_memclear(&si, sizeof(si));
    si.dwXSize = 800;
    si.dwYSize = 600;
    si.dwXCountChars = 80;
    si.dwYCountChars = 25;
    si.dwFillAttribute = 17;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_MINIMIZE;

    /* Make command line by merging the arguments.
     */
    cmdline_buf = osal_stream_buffer_open(OS_NULL, 0, OS_NULL, 0);
    pos = 0;
    while ((a = argv[pos++]))
    {
        if (a != argv[0]) {
            osal_stream_print_str(cmdline_buf, "\n", OSAL_STREAM_DEFAULT);
        }
        osal_stream_print_str(cmdline_buf, a, OSAL_STREAM_DEFAULT);
    }
    osal_stream_buffer_write(cmdline_buf, "\0", 1, &n_written, OSAL_STREAM_DEFAULT);
    cmdline = osal_stream_buffer_content(cmdline_buf, &cmdline_bytes);

    /* Convert to UTF16 for Windows API.
     */
    file_utf16 = osal_str_utf8_to_utf16_malloc(file, &file_utf16_sz);
    cmdline_utf16 = osal_str_utf8_to_utf16_malloc(cmdline, &cmdline_utf16_sz);

    os_memclear(&pi, sizeof(pi));
    rval = CreateProcessW(file_utf16,
        cmdline_utf16, NULL, NULL, FALSE,
        CREATE_NEW_CONSOLE|NORMAL_PRIORITY_CLASS,
        NULL, NULL, &si, &pi);

    if (!rval) {
        rval = CreateProcessW(NULL,
            cmdline_utf16, NULL, NULL, FALSE,
            CREATE_NEW_CONSOLE|NORMAL_PRIORITY_CLASS,
            NULL, NULL, &si, &pi);
    }

    if (!rval) {
        osal_debug_error_str("Starting process failed: ", file);
        s = OSAL_STATUS_CREATE_PROCESS_FAILED;
    }
    else {
        //  WE DO NOT WAIT FOR WaitForSingleObject( pi.hProcess, INFINITE );
        // Getting exit status missing
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }

    os_free(file_utf16, file_utf16_sz);
    os_free(cmdline_utf16, cmdline_utf16_sz);
   
    osal_stream_close(cmdline_buf, OSAL_STREAM_DEFAULT);
    return s;
}

#endif
