/**

  @file    stream/common/osal_stream_defaults.h
  @brief   Stream API default function implementations
  @author  Pekka Lehtikoski
  @version 1.0
  @date    6.2.2020

  A stream implementation may not need to implement all stream functions. The default
  implementations here can be used for to fill in those places in stream interface structure,
  or called from stream's own function to handle general part of the job.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_STREAM_DEFAULTS_H_
#define OSAL_STREAM_DEFAULTS_H_
#include "eosalx.h"

osalStream osal_stream_default_accept(
    osalStream stream,
    os_char *remote_ip_addr,
    os_memsz remote_ip_addr_sz,
    osalStatus *status,
    os_int flags);

osalStatus osal_stream_default_flush(
    osalStream stream,
    os_int flags);

osalStatus osal_stream_default_seek(
    osalStream stream,
    os_long *pos,
    os_int flags);

osalStatus osal_stream_default_write_value(
    osalStream stream,
    os_ushort c,
    os_int flags);

osalStatus osal_stream_default_read_value(
    osalStream stream,
    os_ushort *c,
    os_int flags);

os_long osal_stream_default_get_parameter(
    osalStream stream,
    osalStreamParameterIx parameter_ix);

void osal_stream_default_set_parameter(
    osalStream stream,
    osalStreamParameterIx parameter_ix,
    os_long value);

osalStatus osal_stream_default_select(
    osalStream *streams,
    os_int nstreams,
    osalEvent evnt,
    os_int timeout_ms,
    os_int flags);

#endif
