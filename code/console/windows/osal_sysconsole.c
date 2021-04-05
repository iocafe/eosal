/**

  @file    console/linux/osal_sysconsole.c
  @brief   Operating system default console IO.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  The osal_sysconsole_write() function writes text to the console or serial port designated
  for debug output, and the osal_sysconsole_read() prosesses input from system console or
  serial port.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#include <wchar.h>
#include <conio.h>
#include <stdlib.h>

#if OSAL_CONSOLE

/**
****************************************************************************************************

  @brief Inititalize system console.
  @anchor osal_sysconsole_initialize

  The osal_sysconsole_initialize() function should do any initialization necessary to use the
  system console, for example to set serial port.

  @return  None

****************************************************************************************************
*/
void osal_sysconsole_initialize(
    void)
{
}


#if OSAL_PROCESS_CLEANUP_SUPPORT
/**
****************************************************************************************************

  @brief Shut down system console.
  @anchor osal_sysconsole_shutdown

  The osal_sysconsole_shutdown() function restores console state as it was.
  @return  None

****************************************************************************************************
*/
void osal_sysconsole_shutdown(
    void)
{
}
#endif


/**
****************************************************************************************************

  @brief Write text to system console.
  @anchor osal_sysconsole_write

  The osal_sysconsole_write() function writes a string to process'es default console, if any.

  @param   text Pointer to string to write.

  @return  Pointer to the allocated memory block, or OS_NULL if the function failed (out of
           memory).

****************************************************************************************************
*/
void osal_sysconsole_write(
    const os_char *text)
{
    os_ushort
        *utf16_str;

    os_memsz
        sz;

    /* Convert the string from UTF8 to UTF16. Allocate buffer with osal_sysmem_alloc(),
       (do NOT use os_malloc(), that calls os_lock().
     */
    sz = osal_str_utf8_to_utf16(OS_NULL, 0, text);
    utf16_str = (os_ushort*)osal_sysmem_alloc(sz*sizeof(os_ushort), OS_NULL);
    osal_str_utf8_to_utf16(utf16_str, sz, text);

    /* Print it and release buffer
     */
    wprintf(L"%ls", utf16_str);
    osal_sysmem_free(utf16_str, sz*sizeof(os_ushort));

#if 0
THIS IS WINDOWS IMPLEMENTATION. DOESN'T WORK WELL AT LEAST ON WIN 10
    os_ushort
        *utf16_str;

    os_memsz
        sz,
        n_chars;

    HANDLE
        handle;

    DWORD
        n_written;

    handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) return;

    SetConsoleMode(handle, ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT);

    /* Convert an UTF8 string to UTF16 string in newly allocated buffer and calculate
       number of UTF8 workds.
     */
    utf16_str = osal_str_utf8_to_utf16_malloc(text, &sz);
    n_chars = sz / sizeof(os_ushort) - 1;

    /* Write the UTF16 string to console.
     */
    WriteConsoleW(handle, utf16_str, (DWORD)n_chars, &n_written, NULL);

    /* Release memory allocated for conversion.
     */
    os_free(utf16_str, sz);
#endif
}


/**
****************************************************************************************************

  @brief Read character from system console.
  @anchor osal_sysconsole_run

  The osal_sysconsole_read() function reads the input from system console. If there are any
  input, the callbacks monitoring the input from this console will get called.

  @return  UTF32 character or 0 if no character to read.

****************************************************************************************************
*/
os_uint osal_sysconsole_read(
    void)
{
    int c;
    if (_kbhit())
    {
        c = _getch();
        if (c == '\r') c ='\n';
        return c;
    }
    return 0;
}

#endif
