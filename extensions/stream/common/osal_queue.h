/**

  @file    stream/common/osal_queue.h
  @brief   Byte queues.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This header file contains function prototypes and definitions for byte queues. A byte
  queue is a stream which can be written to and read from.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_QUEUE_INCLUDED
#define OSAL_QUEUE_INCLUDED
#if 0

/** Stream interface structure for byte queues.
 */
#if OSAL_FUNCTION_POINTER_SUPPORT
extern const osalStreamInterface osal_queue_iface;
#endif


/**
****************************************************************************************************

  @name Queue data structure.

  The osalStream type is pointer to a stream, like stream handle. It is defined as pointer to 
  dummy structure to provide compiler type checking. This sturcture is never really allocated,
  and OSAL functions cast their own stream pointers to osalStream pointers.


****************************************************************************************************
*/
typedef struct osalQueue
{
	/** The stream structure must start with stream header structure. The stream header
	    contains parameters common to every stream. 
	 */
	osalStreamHeader hdr;

	/** Pointer to queue buffer. The sz member of this structure is the size of this buffer 
	    in bytes.
	 */
	os_uchar *qbuf;

	/** Buffer size in bytes. See buf member of this structure.
	 */
	os_memsz sz;

	/** Head index. Position in buffer to which next byte is to be written. Range 0 ... sz-1.
	 */
	os_memsz head;

	/** Tail index. Position in buffer from which next byte is to be read. Range 0 ... sz-1.
	 */
	os_memsz tail;

	/** Stream open flags. Flags which were given to osal_queue_open() function. 
	 */
	os_short open_flags;

	/** Control code support flag. If open parameter string has "ctrl=1", this flag is set.
	 */
	os_boolean ctrl_support;
} 
osalQueue;



/** 
****************************************************************************************************

  @name Queue Functions.

  These functions implement byte queue as stream. These functions can either be called 
  directly or through stream interface.

****************************************************************************************************
 */
/*@{*/

/* Construct a byte queue.
 */
osalStream osal_queue_open(
    const os_char *parameters,
	void *option,
	osalStatus *status,
	os_short flags);

/* Delete the byte queue.
 */
void osal_queue_close(
	osalStream stream);

/* Flush the byte queue.
 */
osalStatus osal_queue_flush(
	osalStream stream,
	os_short flags);

/* Write data to byte queue.
 */
osalStatus osal_queue_write(
	osalStream stream,
	const os_uchar *buf,
	os_memsz n,
	os_memsz *n_written,
	os_short flags);

/* Read data from byte queue.
 */
osalStatus osal_queue_read(
	osalStream stream,
	os_uchar *buf,
	os_memsz n,
	os_memsz *n_read,
	os_short flags);

/* Write a byte to byte queue.
 */
osalStatus osal_queue_write_value(
	osalStream stream,
	os_ushort c,
	os_short flags);

/* Read a byte from byte queue.
 */
osalStatus osal_queue_read_value(
	osalStream stream,
	os_ushort *c,
	os_short flags);

/* Get byte queue parameter.
 */
os_long osal_queue_get_parameter(
	osalStream stream,
	osalStreamParameterIx parameter_ix);

/* Set byte queue parameter.
 */
void osal_queue_set_parameter(
	osalStream stream,
	osalStreamParameterIx parameter_ix,
	os_long value);

/*@}*/

#endif
#endif
