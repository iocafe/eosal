/**

  @file    ringbuf/common/osal_ringbuf.c
  @brief   Ring buffer.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Buffer data for communication.
  Transfer data from thread to another.

  Memory allocation handled by caller.

  Atomic changes to head and tail and tranfer data from thread to another. 8. 16. 32 bit processors.
  Multithreading. Buffer size limit, or synchronization needed.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_RING_BUFFER_SUPPORT


typedef struct osalRingBuf
{
    /** Ring buffer, OS_NULL if not used.
     */
    os_char *buf;

    /** Buffer allocation size in bytes. Maximum number of bytes to buffer is buf_sz - 1.
     */
    os_int buf_sz;

    /** Head index. Position in buffer to which next byte is to be written. Range 0 ... buf_sz - 1.
     */
    os_int head;

    /** Tail index. Position in buffer from which next byte is to be read. Range 0 ... buf_sz - 1.
     */
    os_int tail;
}
osalRingBuf;

/* Check if ring buffer is empty.
 */
#define osal_ringbuf_is_empty(r) ((r)->head == (r)->tail)

/* Number of bytes in ring buffer.
 */
#define osal_ringbuf_bytes(r) ((r)->head >= (r)->tail ? (r)->head - (r)->tail : (r)->buf_sz + (r)->head - (r)->tail)

/* Number of continuous bytes in ring buffer to get.
 */
#define osal_ringbuf_continuous_bytes(r) ((r)->head >= (r)->tail ? (r)->head - (r)->tail : (r)->buf_sz - (r)->tail)

/* Free space in ring buffer.
 */
#define osal_ringbuf_space(r) ((r)->buf_sz - osal_ringbuf_bytes(r) - 1)

/* Free continuous space in ring buffer to place new data.
 */
#define osal_ringbuf_continuous_space(r) ((r)->tail > (r)->head ? (r)->tail - (r)->head - 1 : (r)->buf_sz - (r)->head - ((r)->tail ? 0 : 1))

/* Start ring buffer from beginning. Used when ring buffer is becomes empty to prevent unnecessary split by wrap
   around. This MUST NOT be used if ring buffer is used to move data from thread to another.
 */
#define osal_ringbuf_reset(r) (r)->head = (r)->tail = 0;


/* Save buffer pointer and size within ring buffer srructure.
 */
os_int osal_ringbuf_set_buffer(
    osalRingBuf *r,
    const os_char *buf,
    os_int sz);


/* Place up to n bytes into ring buffer. Returns number of bytes placed into ring buffer.
 */
os_int osal_ringbuf_put(
    osalRingBuf *r,
    const os_char *data,
    os_int n);

/* Get data from ring buffer.
 */
os_int osal_ringbuf_get(
    osalRingBuf *r,
    os_char *data,
    os_int n);


#endif
