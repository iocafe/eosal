/**

  @file    serial/common/osal_serial.h
  @brief   OSAL Serials API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This header file contains function prototypes and definitions for OSAL serials API.
  Osal serials api is wrapper for operating system serials.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_SERIAL_INCLUDED
#define OSAL_SERIAL_INCLUDED

#if OSAL_SERIAL_SUPPORT

/** Stream interface structure for serials.
 */
#if OSAL_FUNCTION_POINTER_SUPPORT
extern osalStreamInterface osal_serial_iface;
#endif

/** Define to get serial interface pointer. The define is used so that this can
    be converted to function call.
 */
#define OSAL_SERIAL_IFACE &osal_serial_iface

/* Needed for Windows: Maximum number of socket streams to pass as an argument to 
   osal_serial_select().
 */
#define OSAL_SERIAL_SELECT_MAX 8

/** 
****************************************************************************************************

  @name OSAL Serial Functions.

  These functions implement serials as OSAL stream. These functions can either be called
  directly or through stream interface.

****************************************************************************************************
 */
/*@{*/

/* Open serial.
 */
osalStream osal_serial_open(
    const os_char *parameters,
	void *option,
	osalStatus *status,
	os_int flags);

/* Close serial.
 */
void osal_serial_close(
	osalStream stream);

/* Accept connection from listening serial.
 */
osalStream osal_serial_accept(
	osalStream stream,
	osalStatus *status,
	os_int flags);

/* Flush written data to serial.
 */
osalStatus osal_serial_flush(
	osalStream stream,
	os_int flags);

/* Write data to serial.
 */
osalStatus osal_serial_write(
	osalStream stream,
	const os_uchar *buf,
	os_memsz n,
	os_memsz *n_written,
	os_int flags);

/* Read data from serial.
 */
osalStatus osal_serial_read(
	osalStream stream,
	os_uchar *buf,
	os_memsz n,
	os_memsz *n_read,
	os_int flags);

/* Get serial parameter.
 */
os_long osal_serial_get_parameter(
	osalStream stream,
	osalStreamParameterIx parameter_ix);

/* Set serial parameter.
 */
void osal_serial_set_parameter(
	osalStream stream,
	osalStreamParameterIx parameter_ix,
	os_long value);

/* Wait for new data to read, time to write or operating system event, etc.
 */
#if OSAL_SERIAL_SELECT_SUPPORT
osalStatus osal_serial_select(
	osalStream *streams,
    os_int nstreams,
	osalEvent evnt,
	osalSelectData *selectdata,
    os_int timeout_ms,
	os_int flags);
#endif

/* Initialize OSAL serials library.
 */
void osal_serial_initialize(
	void);

/* Shut down OSAL serials library.
 */
void osal_serial_shutdown(
	void);

/*@}*/


#else

/* No serial port support, define empty socket macros that we do not need to #ifdef code.
 */
#define osal_serial_initialize()
#define osal_serial_shutdown()

#endif
#endif
