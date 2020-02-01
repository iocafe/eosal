/**

  @file    stringx/common/osal_str_to_bin.c
  @brief   Helper function to convert string to binary MAC or IP address.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    1.2.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_STRINGX_SUPPORT

/**
****************************************************************************************************

  @brief Convert string to list of numbers.
  @anchor osal_str_to_bin

  The osal_str_to_bin() converts string representation of MAC or IP address to binary.

  @param   x Pointer to array into which to store the parsed numbers in order.
           This is ushort type to allow IP v6 addresses.
  @param   n Size of x in bytes. 4 or 6 bytes.
  @param   str Input: MAC or IP address as string.
  @param   c Separator character: typically '.' for IP or '-' for MAC.
  @param   b Base: 10 for decimal numbers (IP address) or 16 for hexadecimal numbers (MAC).
  @return  How many numbers were successfully parsed.

****************************************************************************************************
*/
os_int osal_str_to_bin(
    os_ushort *x,
    os_short n,
    const os_char* str,
    os_char c,
    os_short b)
{
    os_long v;
    os_memsz bytes;
    os_int count;

    count = 0;

    while (OS_TRUE)
    {
        if (b == 16)
        {
            v = osal_hex_str_to_int(str, &bytes);
        }
        else
        {
            v = osal_str_to_int(str, &bytes);
        }

        if (bytes)
        {
            x[count++] = (os_ushort)v;
        }

        if (--n <= 0 || !bytes) break;

        str = os_strchr((os_char*)str, c);
        if (str == OS_NULL) break;
        ++str;
    }

    return count;
}

#endif
