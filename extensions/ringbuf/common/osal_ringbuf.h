/**

  @file    ringbuf/common/osal_ringbuf.c
  @brief   Ring buffer.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Ring buffers are commonly used to buffer data for communication, to combine multiple small
  writes into one TCP package, or transfer data from thread to another.

  The osal_rinngbuf is simple general purpose ring buffer implementation, with typical head and
  tail indices. The ting buffer state is maintained in osalRingBuf structure (head, tail, buffer
  pointer and size)

  Memory allocation for buffer handled by caller and stored into buf, and buf_sz members
  of ring buffer state structure.

  Atomic changes to head and tail and tranfer data from thread to another: 8, 16, 32, 64 bit
  processors use integers up to this size atomically (access and set is one processor
  instruction and synchronization is not needed). This ring buffer doesn't require synchronization
  when moving data between threads, if buffer size less than 65536 for 16 bit processors and
  0x7FFFFFFF for 32 bit processors.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_RING_BUFFER_SUPPORT

/* Ring buffer state structure. Before using ring buffer, application needs to set buffer pointer
   and size, and clear head and tail indeixes to zero.
 */
typedef struct osalRingBuf
{
    /** Ring buffer.
     */
    os_char *buf;

    /** Buffer allocation size in bytes. Maximum number of buffered bytes is buf_sz - 1.
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

/* Check if ring buffer is full.
 */
#define osal_ringbuf_is_full(r) ((r)->head + 1 == (r)->tail || ((r)->head == (r)->buf_sz - 1 && (r)->tail == 0))

/* Number of bytes in ring buffer.
 */
#define osal_ringbuf_bytes(r) ((r)->head >= (r)->tail ? (r)->head - (r)->tail : (r)->buf_sz + ((r)->head - (r)->tail))

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
#define osal_ringbuf_reset(r) (r)->head = (r)->tail = 0


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

/* Reorganize data in ring buffer to be continuous.
 */
void osal_ringbuf_make_continuous(
    osalRingBuf *r);

#endif
