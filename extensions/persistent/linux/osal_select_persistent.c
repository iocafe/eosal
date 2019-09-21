/**

  @file    persistent/linux/osal_select_persistent.c
  @brief   Select persistent storage implementation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.9.2019

  The linux/osal_select_persistent.c selects persistent storage implementation to use with
  linux platform, by including the implementation code.

  Micro-controllers store parameters persistent storage. We emulate this on linux and Windows by
  saving persistent parameters into files. Since multiple platforms may use the same file
  system implementation, we do not write it here, but include the shared implementation.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_PERSISTENT_INCLUDED
#define OSAL_PERSISTENT_INCLUDED
#if OSAL_OSAL_PERSISTENT_SUPPORT
#include "extensions/persistent/shared/filesystem/osal_fsys_persistent.c"
#endif
#endif
