/**

  @file    stringx/common/osal_ip_from_str.c
  @brief   Convert string to binary IP address.
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

  @brief Convert string to binary IP address.
  @anchor osal_ip_from_str

  The osal_ip_from_str() converts string representation of IP address to binary. If the function
  fails, binary IP address is left unchanged.

  String IP address can be either IP V4 address in "192.168.1.222" format or
  IP v6 address in "2001:0db8:85a3:0000:0000:8a2e:0370:7334" format.

  The IPv4-mapped IPv6 address 192.0.2.128 ::ffff:c000:0280 is written as ::ffff:192.0.2.128,
  thus expressing clearly the original IPv4 address that was mapped to IPv6.

  @param   ip Pointer where to store IP v4 as binary, 4 bytes. Or v6 16 bytes.
  @param   ip_sz IP buffer size in bytes: 4 to accept IP v4 addressess or 16 to accept
           both IP v6 and IPv4 addresses.
  @param   str Input, IP address as string.
  @return  OSAL_SUCCESS if IP v4 address successfully parsed. Value OSAL_IS_IPV6
           indicate that IP v6 address was succesfully parsed. Other values indicate
           that str could not be interpreted.

****************************************************************************************************
*/
osalStatus osal_ip_from_str(
    os_uchar *ip,
    os_memsz ip_sz,
    const os_char *str)
{
    os_ushort buf[8];
    os_int count, i;

    os_memclear(ip, ip_sz);
    if (str == OS_NULL) return OSAL_STATUS_FAILED;

    if (ip_sz >= 16)
    {
        count = osal_str_to_bin(buf, 8, str, ':', 16);
        if (count == 8)
        {
            for (i = 0; i < 8; i++)
            {
                *(ip++) = (os_uchar)(buf[i] >> 8);
                *(ip++) = (os_uchar)buf[i];
            }
            return OSAL_IS_IPV6;
        }
    }

    count = osal_str_to_bin(buf, 4, str, '.', 10);
    if (count == 4)
    {
        for (i = 0; i < 4; i++)
        {
            *(ip++) = (os_uchar)buf[i];
        }
        return OSAL_SUCCESS;
    }

    osal_debug_error("IP string error");
    return OSAL_STATUS_FAILED;
}

#endif
