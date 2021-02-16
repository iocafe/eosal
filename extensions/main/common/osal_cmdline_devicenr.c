/**

  @file    main/common/osal_cmdline_devicenr.c
  @brief   Function to override saved device number from command line.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_MAIN_SUPPORT

/**
****************************************************************************************************

  @brief Select device number to use.
  @anchor osal_cmdline_devicenr

  If we are not running in microcontroller, we may want to allow setting device number
  from command line, like "-n=7". This function checks if device number is specified on
  command line arguments argx/argv. If so, it will be returned. If not, device number
  given as argument is returned.

  @return  Selected device number.

****************************************************************************************************
*/
os_int osal_command_line_device_nr(
    os_int device_nr,
    os_int argc,
    os_char *argv[])
{
    os_int i;

    for (i = 1; i < argc; i++) {
        if (!os_strncmp(argv[i], "-n=", 3)) {
            if (osal_char_isdigit(argv[i][3])) {
                device_nr = (os_int)osal_str_to_int(argv[i] + 3, OS_NULL);
                break;
            }
        }
    }
    return device_nr;
}


#endif
