/**

  @file    stringx/common/osal_str_to_bin.c
  @brief   Convert MAC address from string to binary.
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

  @brief Convert string to binary MAC address.
  @anchor osal_mac_from_str

  The osal_mac_from_str() converts string representation of MAC address to binary.
  If the function fails, binary MAC is left unchanged.

  @param   mac Pointer to byte array into which to store the MAC, 6 bytes.
  @param   str Input, MAC address as string.
  @return  OSAL_SUCCESS if successfull, other return values indicate that the MAC address
           string count not be interprented.

****************************************************************************************************
*/
osalStatus osal_mac_from_str(
    os_uchar mac[6],
    const char* str)
{
    os_ushort buf[6];
    os_int count, i;

    count = osal_str_to_bin(buf, 6, str, '-', 16);
    if (count == 6)
    {
        for (i = 0; i < 6; i++)
        {
            *(mac++) = (os_uchar)buf[i];
        }
        return OSAL_SUCCESS;
    }

    osal_debug_error("MAC string error");
    return OSAL_STATUS_FAILED;
}

#endif
