/**

  @file    bluetooth/common/osal_bluetooth.h
  @brief   OSAL Bluetooths API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This header file contains function prototypes and definitions for OSAL bluetooths API.
  Osal bluetooths api is wrapper for operating system bluetooths.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_BLUETOOTH_INCLUDED
#define OSAL_BLUETOOTH_INCLUDED

#if OSAL_BLUETOOTH_SUPPORT

/** Stream interface structure for bluetooths.
 */
extern const osalStreamInterface osal_bluetooth_iface;

/** Define to get bluetooth interface pointer. The define is used so that this can
    be converted to function call.
 */
#define OSAL_BLUETOOTH_IFACE &osal_bluetooth_iface


/** 
****************************************************************************************************
  OSAL bluetooth initialization and shutdown.
****************************************************************************************************
*/

/* Initialize OSAL bluetooths library.
 */
void osal_bluetooth_initialize(
	void);

/* Shut down OSAL bluetooths library.
 */
void osal_bluetooth_shutdown(
	void);

#else

/* No bluetooth port support, define empty socket macros that we do not need to #ifdef code.
 */
#define osal_bluetooth_initialize()
#define osal_bluetooth_shutdown()

/* No bluetooth interface, allow build even if the define is used.
 */
#define OSAL_BLUETOOTH_IFACE OS_NULL

#endif
#endif
