/**

  @file    filesys/common/osal_filectrl.h
  @brief   File functions, like delte file, etc.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_FILESYS_SUPPORT

/** Delete a file.
 */
osalStatus osal_remove(
    const os_char *path,
    os_int flags);

#endif
