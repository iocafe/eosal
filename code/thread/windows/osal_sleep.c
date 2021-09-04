/**

  @file    thread/windows/osal_sleep.c
  @brief   Creating, terminating, scheduling and identifying threads.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Thread functions for Windows are implemented here.
  A process can run multiple tasks concurrently, and these concurrently running tasks are called
  threads of execution. The treads of the same process and share memory and other processe's
  resources. Typically access to shared resources must be synchronized, see mutexes.
  On a single processor the processor switches between different threads. This context switching
  generally happens frequently enough that the user perceives the threads or tasks as running
  at the same time. On a multiprocessor or multi-core system, the threads or tasks will generally
  run at the same time, with each processor or core running a particular thread or task.
  A new thread is created by osal_thread_create() function call
  Thread priorizing and sleep are handled by osal_thread_set_priority() and
  osal_sleep() functions.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#ifdef OSAL_WINDOWS

/**
****************************************************************************************************

  @brief Suspend thread execution for a specific time, milliseconds.
  @anchor osal_sleep

  The osal_sleep() function suspends the execution of the current thread for a specified
  interval. The function is used for both to create timed delays and to force scheduler to give
  processor time to lower priority threads. If time_ms is zero the function suspends execution
  of the thread until end of current processor time slice.

  @param   time_ms Time to sleep, milliseconds. Value 0 sleeps until end of current processor
           time slice.
  @return  None.

****************************************************************************************************
*/
void osal_sleep(
    os_long time_ms)
{
    /* Call Windows to sleep.
     */
    Sleep((DWORD)time_ms);
}


/**
****************************************************************************************************

  @brief Suspend thread execution for a specific time, microseconds.
  @anchor os_microsleep

  The os_microsleep() function suspends the execution of the current thread for a specified
  interval. The function is used for both to create timed delays and to force scheduler to give
  processor time to lower priority threads. If time_ms is zero the function suspends execution
  of the thread until end of current processor time slice.

  Windows specific: The function support only one millisecond precision.

  @param   time_us Time to sleep, microseconds. Value 0 sleeps until end of current processor
           time slice.
  @return  None.

****************************************************************************************************
*/
void os_microsleep(
    os_long time_us)
{
    /* Call Windows to sleep.
     */
    Sleep((DWORD)((time_us + 999) / 1000));
}

#endif