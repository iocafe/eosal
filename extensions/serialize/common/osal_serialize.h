/**

  @file    serialize/common/osal_serialize.h
  @brief   Convert floating point number to two integers and vice versa.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_SERIALIZE_H_
#define OSAL_SERIALIZE_H_
#include "eosalx.h"

#if OSAL_SERIALIZE_SUPPORT

/**
****************************************************************************************************

  @name Integer serialization functions.

  These functions pack integers to serial format.

****************************************************************************************************
*/
/*@{*/

/* Recommended minimum size for buffer argument to osal_intser_writer().
 */
#define OSAL_INTSER_BUF_SZ 10

/* Convert integer to packed serial format.
 */
os_int osal_intser_writer(
    os_char *buf,
    os_long x);

/* Get integer from packed serial format.
 */
os_int osal_intser_reader(
    const os_char *buf,
    os_long *x);

/* Get number of bytes following first byte
 */
#define osal_intser_more_bytes(c) ((os_int)(((os_uchar)(c)) >> 4))


/*@}*/


/**
****************************************************************************************************

  @name Integer serialization functions.

  These functions pack integers to serial format.

****************************************************************************************************
*/
/*@{*/

/* Convert integer mantissa and exponent to double.
 */
os_boolean osal_ints2double(
    os_double *x,
    os_long m,
    os_short e);

/* Split double to mantissa and exponent.
 */
void osal_double2ints(
    os_double x,
    os_long *m,
    os_short *e);

/* Convert integer mantissa and exponent to float.
 */
os_boolean osal_ints2float(
    os_float *x,
    os_long m,
    os_short e);

/* Split float to mantissa and exponent.
 */
void osal_float2ints(
    os_float x,
    os_long *m,
    os_short *e);

/*@}*/

#endif
#endif
