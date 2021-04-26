/**

  @file    stream/common/osal_stream_defaults.c
  @brief   Stream API default function implementations
  @author  Pekka Lehtikoski
  @version 1.0
  @date    6.2.2020

  Most of these functions do nothing, just place holders to avoid calling NULL function.

  A stream implementation may not need to implement all stream functions. The default
  implementations here can be used for to fill in those places in stream interface structure,
  or called from stream's own function to handle general part of the job.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"

osalStream osal_stream_default_accept(
    osalStream stream,
    os_char *remote_ip_addr,
    os_memsz remote_ip_addr_sz,
    osalStatus *status,
    os_int flags)
{
    if (status) *status = OSAL_STATUS_FAILED;
    if (remote_ip_addr) *remote_ip_addr = '\0';
    return OS_NULL;
}


osalStatus osal_stream_default_flush(
    osalStream stream,
    os_int flags)
{
    return OSAL_SUCCESS;
}

osalStatus osal_stream_default_seek(
    osalStream stream,
    os_long *pos,
    os_int flags)
{
    return OSAL_STATUS_FAILED;
}

osalStatus osal_stream_default_select(
    osalStream *streams,
    os_int nstreams,
    osalEvent evnt,
    os_int timeout_ms,
    os_int flags)
{
    /* Return value OSAL_STATUS_NOT_SUPPORTED indicates that select is not implemented.
     */
    return OSAL_STATUS_NOT_SUPPORTED;
}

