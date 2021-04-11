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


/**
****************************************************************************************************

  @brief Get data from ring buffer.

  Get up to n bytes of data from ring buffer.

  @param   r Ring buffer state structure.
  @param   data Pointer to the buffer where to place the data from ring buffer.
  @param   n Maximum number of bytes of data to get.
  @return  Number of bytes read from the ring buffer.

****************************************************************************************************
*/
os_int osal_ringbuf_get(
    osalRingBuf *r,
    os_char *data,
    os_int n)
{
    os_int n_now, n_now2, n_continuous, head, tail;
    const os_char *buf;

    n_now = n;
    n_continuous = osal_ringbuf_continuous_bytes(r);
    if (n_continuous < n_now) {
        n_now = n_continuous;
    }
    if (n_now <= 0) {
        return 0;
    }

    buf = r->buf;
    tail = r->tail;
    os_memcpy(data, buf + tail, n_now);
    tail += n_now;
    if (tail < r->buf_sz) {
        r->tail = tail;
        return n_now;
    }

    n_now2 = n - n_now;
    head = r->head;
    if (n_now2 > head) {
        n_now2 = head;
    }
    if (n_now2 <= 0) {
        r->tail = 0;
        return n_now;
    }
    os_memcpy(data, buf, n_now2);
    r->tail = n_now2;
    return n_now + n_now2;
}


/**
****************************************************************************************************

  @brief Put data to ring buffer.

  Place up to n bytes into ring buffer.

  @param   r Ring buffer state structure.
  @param   data Pointer to the beginning of data put into ring buffer.
  @param   n Maximum number of bytes to place into ring buffer.
  @return  Number of bytes placed into ring buffer.

****************************************************************************************************
*/
os_int osal_ringbuf_put(
    osalRingBuf *r,
    const os_char *data,
    os_int n)
{
    os_char *buf;
    os_int head, tail, n_now, n_now2, n_continuous;

    n_now = n;
    n_continuous = osal_ringbuf_continuous_space(r);
    if (n_continuous < n_now) {
        n_now = n_continuous;
    }
    if (n_now <= 0) {
        return 0;
    }

    buf = r->buf;
    head = r->head;

    os_memcpy(buf + head, data, n_now);
    head += n_now;
    if (head < r->buf_sz) {
        r->head = head;
        return n_now;
    }
    tail = r->tail;
    if (n_now == n || tail <= 1) {
       r->head = 0;
       return n_now;
    }

    data += n_now;
    n_now2 = n - n_now;
    if (n_now2 > tail - 1) {
        n_now2 = tail - 1;
    }

    os_memcpy(buf, data, n_now2);
    r->head = n_now2;
    return n_now + n_now2;
}


/**
****************************************************************************************************

  @brief Reorganize data in ring buffer to be continuous.

  Rotate ring buffer so that all buffered data is in continous memory.

  Warning: This function cannot be used if ring buffer is used to move data from thread to
  another (unless syncronozation is used).

  Warning: This function should not be used in small microcontroller socket wrappers.
  It uses > 1420 bytes of stack.

  @param   r Ring buffer state structure.

****************************************************************************************************
*/
void osal_ringbuf_make_continuous(
    osalRingBuf *r)
{
    if (r->head < r->tail)
    {
        os_char tmpbuf[r->buf_sz], *buf;
        os_int n;

        buf = r->buf;

        n = r->buf_sz - r->tail;
        os_memcpy(tmpbuf, buf + r->tail, n);
        os_memcpy(tmpbuf + n, buf, r->head);
        r->tail = 0;
        r->head += n;
        os_memcpy(buf, tmpbuf, r->head);
    }
}


#endif
