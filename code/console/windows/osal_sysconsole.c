/**

  @file    console/linux/osal_sysconsole.c
  @brief   Operating system default console IO.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  The osal_sysconsole_write() function writes text to the console or serial port designated
  for debug output, and the osal_sysconsole_read() prosesses input from system console or
  serial port.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
/* #include <windows.h> */
#include <wchar.h>
#include <conio.h>
#include <stdio.h>

#if OSAL_CONSOLE

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

	/* Convert an UTF8 string to UTF16 string, print it
       and release allocated buffer.
	 */
	utf16_str = osal_string_utf8_to_utf16_malloc(text, &sz);
    wprintf(L"%s", utf16_str); 
	os_free(utf16_str, sz);

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
	utf16_str = osal_string_utf8_to_utf16_malloc(text, &sz);
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
    if (_kbhit())
    {
        return getchar();
    }
    return 0;
#if 0
	os_uint
		c;

	os_uint
		*buf,
        rval;

	os_int
		i,
		n;

	HANDLE 
		handle;

	DWORD
		n_read;

	INPUT_RECORD
		irecord[OSAL_CONSOLE_BUFFER_CHARS];

	handle = GetStdHandle(STD_INPUT_HANDLE);
	if (handle == INVALID_HANDLE_VALUE) return 0;

	// SetConsoleMode(handle, ENABLE_PROCESSED_INPUT);

	/* Peek if there is any data in console input buffer.
	 */
	if (!GetNumberOfConsoleInputEvents(handle, &n_read))
	{
		return 0;
	}

	/* If no data in console input buffer, then do nothing.
	 */
	if (!n_read) return 0;

    buf = osal_global->constate.buf;

	/* Read up to OSAL_CONSOLE_BUFFER_CHARS items from console input buffer.
	 */
	if (ReadConsoleInputW(handle, irecord, OSAL_CONSOLE_BUFFER_CHARS, &n_read))
	{
		n = 0;
        while (buf[n]) 
        {
            if (++n >= OSAL_CONSOLE_BUFFER_CHARS) goto processbuf;
        }
 
		for (i = 0; i < (os_int)n_read; ++i)
		{
			switch (irecord[i].EventType)
			{
				case KEY_EVENT:
					c = irecord[i].Event.KeyEvent.uChar.UnicodeChar;
					if (c && irecord[i].Event.KeyEvent.bKeyDown)
					{
						if (c == '\r') c = '\n';
						buf[n++] = (os_uint)c;
                        if (n >= OSAL_CONSOLE_BUFFER_CHARS) goto processbuf;
					}
					break;

				default:
					break;
			}
		}
	}

processbuf:
    /* Return character from buffer, if any.
     */
    rval = buf[0];
    if (rval) 
    {
        for (i = 0; i<OSAL_CONSOLE_BUFFER_CHARS-1; i++)
        {
            if (buf[i+1] == 0) break;
            buf[i] = buf[i+1];
        }
        buf[i] = 0;
    }
    return rval;
#endif
}

#endif
