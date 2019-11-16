/**

  @file    thread/metal/osal_thread_priority.c
  @brief   Thread priority and identification.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2017

  Thread priority and identification for plain metal.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"

#if OSAL_MULTITHREAD_SUPPORT

#include <FreeRTOS.h>
#include <task.h>


/* Forward referred static functions.
 */
static UBaseType_t osal_thread_priority_to_rt_priority(
    osalThreadPriority priority);

/**
****************************************************************************************************

  @brief Set thread priority.
  @anchor osal_thread_set_priority

  The osal_thread_set_priority() function sets current thread priority. Thread priority will
  set how operating system scheduler should share time between threads. Most systems we work
  on implement preemptive multitasking. This mean that threads running on higher priority get
  processor time first, and only when no higher priority thread needs time then the lower
  priority threads will receive it. Multiprocessor or multi-core environment will change this,
  since one processor or core can be executing only one task.

  The OSAL_THREAD_PRIORITY_LOW, OSAL_THREAD_PRIORITY_NORMAL and OSAL_THREAD_PRIORITY_HIGH are
  used to prioritize execution of normal threads. The OSAL_THREAD_PRIORITY_TIME_CRITICAL is
  reserved for real time tasks only, and using this priority will put special requirements
  on the thread.
  
  @param   priority Priority to set, one of OSAL_THREAD_PRIORITY_LOW, OSAL_THREAD_PRIORITY_NORMAL,
		   OSAL_THREAD_PRIORITY_HIGH or OSAL_THREAD_PRIORITY_TIME_CRITICAL.
           See @ref osalThreadPriority "enumeration of thread priorities" for more information.

  @return  If the thread priority is  succesfully set, the function returns OSAL_SUCCESS. If
           operating system fails to set thread priority, the function returns
           OSAL_STATUS_THREAD_SET_PRIORITY_FAILED. Basically all nonzero return values indicate
           an error. See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
osalStatus osal_thread_set_priority(
	osalThreadPriority priority)
{
    /* Get current thread handle and set thread priority.
     */
    vTaskPrioritySet(xTaskGetCurrentTaskHandle(), osal_thread_priority_to_rt_priority(priority));

    /* Success.
     */
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Convert OSAL thread priority to FreeRTOS thread priority.

  The osal_thread_priority_to_rt_priority function converts OSAL thread priority to linux
  thread priority number. For portability the OSAL has it's own thread priority enumeration,
  and this function translated these for linux.

  @return  Thread priority. One of OSAL_THREAD_PRIORITY_LOW, OSAL_THREAD_PRIORITY_NORMAL,
           OSAL_THREAD_PRIORITY_HIGH or OSAL_THREAD_PRIORITY_TIME_CRITICAL.
           See @ref osalThreadPriority "enumeration of thread priorities" for more information.

  @return  FreeRTOS thread priority number.

****************************************************************************************************
*/
static UBaseType_t osal_thread_priority_to_rt_priority(
    osalThreadPriority priority)
{
    int rtpriority;

    switch (priority)
    {
        case OSAL_THREAD_PRIORITY_LOW:
            rtpriority = 1;
            break;

        default:
            osal_debug_error("Unknown thread priority");
            /* continues...
             */

        case OSAL_THREAD_PRIORITY_NORMAL:
            rtpriority = 3;
            break;

        case OSAL_THREAD_PRIORITY_HIGH:
            rtpriority = 5;
            break;

        case OSAL_THREAD_PRIORITY_TIME_CRITICAL:
            rtpriority = 30;
            break;
    }

    return rtpriority;
}


/**
****************************************************************************************************

  @brief Get thread identifier.
  @anchor osal_thread_get_id

  The osal_thread_get_id() function gets thread identifier. Thread identifier is integer number
  uniquely identifying the thread. Thread identifiers can be used for debugging, to make
  sure that accessed resources really belong to the current thread, or to find thread specific
  resources.

  @param   reserved for future. Linux uses multiple different thread identifiers, this may later
           be used which one. Set 0 for now.
  @return  Thread id.

****************************************************************************************************
*/
os_long osal_thread_get_id(
    os_int reserved)
{
    return (os_long)xTaskGetCurrentTaskHandle();
}

#endif
