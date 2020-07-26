/**

  @file    mutex/common/osal_mutex.h
  @brief   Mutexes, synchronizing thread access to shared resources.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  This header file contains functions prototypes and definitions for creating, deleting,
  locking and unlocking mutexes, plus of system mutex functions. Generally mutexes are used 
  to synchronize thread access of shared resources. A mutex is created by osal_mutex_create()
  function and deleted by osal_mutex_delete() function. Before accessing a shared resource
  a thread must call osal_mutex_lock(), and osal_mutex_unlock() once it finishes access 
  to the shared resource. This will cause other threads calling osal_mutex_lock() to block
  until the first thread has finished with the shared resource. An OSAL mutex is always so 
  called recursive mutex.

  System mutex is a global mutex for whole process used to synchronize access to global 
  variables, etc. When system mutex is locked by os_lock() function, the 
  thread is switched to very high priority to prevent priority reversal. The old thread priority
  is restored when system mutex is unlocked by os_unlock() function. Using
  one system mutex (as far as reasonable) instead of many mutexes will prevent deadlock
  situations. Limitation is that System mutex lock time should be minimized to be as short 
  as possible, since system mutex locks may halt many threads. For example never lock system
  mutex when waiting for IO, etc.

  Define OSAL_MULTITHREAD_SUPPORT controls compiler's code generation for supporting mutexes. 
  If the define is nonzero, then code for mutex support is generated, otherwise not.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_MUTEX_H_
#define OSAL_MUTEX_H_
#include "eosal.h"

/**
****************************************************************************************************

  @name Mutex pointer type.

  The osalMutex type is pointer to a mutex. It is defined as pointer to dummy structure solely 
  to provide compiler type checking. This sturcture is never really allocated, and OSAL operating 
  system functions cast their own mutex pointers to osalMutex pointers and vice versa.


****************************************************************************************************
*/
/*@{*/

/* Dummy structure to provide compiler type checking.
 */
struct osalMutexStruct;

/** Mutex pointer. This pointer is returned by osal_mutex_create() function.
 */
typedef struct osalMutexStruct *osalMutex;
/*@}*/


#if OSAL_MULTITHREAD_SUPPORT 

/**
****************************************************************************************************

  @name Mutex Initialization and Shut Down Functions

  These are called internally by OSAL, and should not normally be called by application.
  The osal_initialize() calls osal_mutex_initialize() and osal_shutdown() calls
  osal_mutex_shutdown().

****************************************************************************************************
 */
/*@{*/

/* Initialize mutex support.
 */
void osal_mutex_initialize(
    void);

/* Shut down mutex support.
 */
void osal_mutex_shutdown(
    void);

/*@}*/


/** 
****************************************************************************************************

  @name Mutex Functions

  A mutex is created by osal_mutex_create() function and deleted by osal_mutex_delete() function. 
  Before accessing a shared resource a thread must call osal_mutex_lock(), and osal_mutex_unlock() 
  once it finishes access to the shared resource. 

****************************************************************************************************
 */
/*@{*/

/* Create a new mutex.
 */
osalMutex osal_mutex_create(
    void);

/* Delete an mutex.
 */
void osal_mutex_delete(
    osalMutex mutex);

/* Lock a mutex.
 */
void osal_mutex_lock(
    osalMutex mutex);

/* Unlock a mutex.
 */
void osal_mutex_unlock(
    osalMutex mutex);

/*@}*/


/** 
****************************************************************************************************

  @name System Mutex Functions

  The system mutex is single mutex used to synchronize thread access to global variables, etc. 
  The system mutex is locked by os_lock() function and released by 
  os_unlock() function.

****************************************************************************************************
 */
/*@{*/


/* Lock the system mutex.
 */
void os_lock(
    void);

/* Unlock the system mutex.
 */
void os_unlock(
    void);

/*@}*/


#else

/**
****************************************************************************************************

  @name Empty Mutex Macros

  If OSAL_MULTITHREAD_SUPPORT flag is zero, these macros will replace functions and not generate
  any code. This allows to compile code which has calls mutex functions without multithreading
  support.

****************************************************************************************************
*/
/*@{*/

  #define osal_mutex_create() OS_NULL
  #define osal_mutex_delete(x)
  #define osal_mutex_lock(x)
  #define osal_mutex_unlock(x)
  #define os_lock()
  #define os_unlock()
/*@}*/

#endif

#endif
