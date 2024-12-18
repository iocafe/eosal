/**

  @file    osal_stream_buffer.h
  @brief   Memory buffer with OSAL stream API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Memory buffer, which implements OSAL stream interface. This class presents simple buffer
  in memory as a stream for reading and writing.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_STREAM_BUFFER_H_
#define OSAL_STREAM_BUFFER_H_
#include "eosalx.h"
#if OSAL_STREAM_BUFFER_SUPPORT

/** Stream interface structure for the stream buffer class.
 */
extern const osalStreamInterface osal_stream_buffer_iface;

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

/* Open stream buffer.
 */
osalStream osal_stream_buffer_open(
    const os_char *parameters,
    void *option,
    osalStatus *status,
    os_int flags);

/* Close stream buffer.
 */
void osal_stream_buffer_close(
    osalStream stream,
    os_int flags);

/* Get or set current read or write position.
 */
osalStatus osal_stream_buffer_seek(
    osalStream stream,
    os_long *pos,
    os_int flags);

/* Write data to stream buffer.
 */
osalStatus osal_stream_buffer_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags);

/* Read data from stream buffer.
 */
osalStatus osal_stream_buffer_read(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags);

/* Allocate more memory for the buffer.
 */
osalStatus osal_stream_buffer_realloc(
    osalStream stream,
    os_memsz request_sz);

/* Get pointer to buffer content.
 */
os_char *osal_stream_buffer_content(
    osalStream stream,
    os_memsz *n);

/* Get stream buffer content and take ownership of it.
 */
os_char *osal_stream_buffer_adopt_content(
    osalStream stream,
    os_memsz *n,
    os_memsz *alloc_n);

/*@}*/
#endif

/* No stream buffer interface, allow build even if the define is used.
 */
#ifndef OSAL_STREAM_BUFFER_IFACE
    #define OSAL_STREAM_BUFFER_IFACE OS_NULL
#endif

#endif
