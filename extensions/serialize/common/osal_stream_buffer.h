/**

  @file    osal_stream_buffer.h
  @brief   OSAL stream_buffers.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    25.10.2019

  This header file contains function prototypes and definitions for OSAL stream buffers.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_STREAM_BUFFER_INCLUDED
#define OSAL_STREAM_BUFFER_INCLUDED

/** Stream interface structure for stream_buffers.
 */
#if OSAL_SERIALIZE_SUPPORT
#if OSAL_FUNCTION_POINTER_SUPPORT
extern const osalStreamInterface osal_stream_buffer_iface;
#endif

/** Define to get stream_buffer interface pointer. The define is used so that this can
    be converted to function call.
 */
#define OSAL_STREAM_BUFFER_IFACE &osal_stream_buffer_iface


/** 
****************************************************************************************************

  @name OSAL stream buffer Functions.

  These functions implement stream buffer as OSAL stream. These functions can either be called
  directly or through stream interface.

****************************************************************************************************
 */
/*@{*/

/* Open stream_buffer.
 */
osalStream osal_stream_buffer_open(
    const os_char *parameters,
    void *option,
    osalStatus *status,
    os_int flags);

/* Close stream_buffer.
 */
void osal_stream_buffer_close(
    osalStream stream);

/* Write data to stream_buffer.
 */
osalStatus osal_stream_buffer_write(
    osalStream stream,
    const os_uchar *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags);

/* Read data from stream_buffer.
 */
osalStatus osal_stream_buffer_read(
    osalStream stream,
    os_uchar *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags);

/* Allocate more memory for the buffer.
 */
osalStatus osal_stream_buffer_realloc(
    osalStream stream,
    os_memsz request_sz);
/*@}*/

#else

/* No stream_buffer interface, allow build even if the define is used.
 */
#define OSAL_STREAM_BUFFER_IFACE OS_NULL

#endif
#endif
