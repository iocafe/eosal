/**

  @file    eosal_checksum.h
  @brief   Calculate checksum.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    29.5.2019

  From application's view communication status appears the same as data memory and is accessed
  using the same ioc_read(), ioc_get16(), ioc_write(), ioc_set16(), etc. functions. For data
  memory, the address is positive or zero, status memory addresses are negative.

  Copyright 2018 Pekka Lehtikoski. This file is part of the iocom project and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_CHECKSUM_INCLUDED
#define OSAL_CHECKSUM_INCLUDED

/* Enable/disable checksum test code.
 */
#define OSAL_CHECKSUM_TEST 0

/* Calculates checksum for buffer.
 */
os_ushort os_checksum(
    os_uchar *buf,
    int n);

#if OSAL_CHECKSUM_TEST
/* Test the checksum code.
 */
int osal_test_checksum(
    void);
#endif


#endif