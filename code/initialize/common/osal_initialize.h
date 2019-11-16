/**

  @file    initialize/common/osal_initialize.h
  @brief   OSAL initialization and shut down.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  OSAL library initialization, shut down and pointer to global OSAL state structure.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_INITIALIZE_INCLUDED
#define OSAL_INITIALIZE_INCLUDED


/**
****************************************************************************************************

  @name Pointer to Global OSAL State Structure.

  The state structure holds global OSAL state. The stucture is always accessed trough this
  pointer so that DLLs are able to share the state structure of the process which loaded
  them.

****************************************************************************************************
*/
/*@{*/

/* Pointer to global OSAL state structure.
 */
extern osalGlobalStruct *osal_global;
/*@}*/


/**
****************************************************************************************************

  @name Initialization and Shut Down Functions

  The osal_initialize() function initializes OSAL library for use. This function should
  be the first OSAL function called. The osal_shutdown() function cleans up resources
  used by the OSAL library.

****************************************************************************************************
 */
/*@{*/

/* Flags for osal_initialize() function.
 */
#define OSAL_INIT_DEFAULT 0
#define OSAL_INIT_NO_LINUX_SIGNAL_INIT 1

/* Initialize OSAL library for use.
 */
void osal_initialize(
    os_int flags);

/* Shut down OSAL library, clean up.
 */
void osal_shutdown(
    void);

/* Operating system specific OSAL library initialization.
 */
void osal_init_os_specific(
    os_int flags);

/* Operating system specific OSAL shutdown code.
 */
void osal_shutdown_os_specific(
    void);

/* Reboot the computer.
 */
void osal_reboot(
    os_int flags);

/*@}*/

#endif
