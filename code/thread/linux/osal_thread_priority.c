/**

  @file    thread/linux/osal_thread_priority.c
  @brief   Thread priority.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Thread priority is not used for Linux.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"


/**
****************************************************************************************************

  @brief Set thread priority.
  @anchor osal_thread_set_priority

  The osal_thread_set_priority() function sets current thread priority. Thread priority will
  set how operating system scheduler should share time between threads.

  Linux specific note: Linux thread scheduler does amazingly good job without application
  specific thread priority settings, so these are not supported for now.
  Calling osal_thread_set_priority() does nothing. While it is possible to use real time scheduling
  and set priorities and we may add support for this in eosal,
  I have found this often counterproductive: It requires serious effort and knowledge to get
  better performance than the default linux scheduler provides easily.

  @param   priority Priority to set, ignored in linux.
  @return  Always OSAL_SUCCESS.

****************************************************************************************************
*/
#if OSAL_MULTITHREAD_SUPPORT
osalStatus osal_thread_set_priority(
    osalThreadPriority priority)
{
    return OSAL_SUCCESS;
}
#endif
