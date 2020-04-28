/**

  @file    stream/commmon/osal_supplement.c
  @brief   Functions supplementing stream interface.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    6.2.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"

#if OSAL_SERIALIZE_SUPPORT
osalStatus osal_stream_write_long(
    osalStream stream,
    os_long x,
    os_int flags)
{
    os_char tmp[OSAL_INTSER_BUF_SZ];
    os_memsz n_written;
    os_int tmp_n;
    osalStatus s;

    tmp_n = osal_intser_writer(tmp, x);
    s = osal_stream_write(stream, tmp, tmp_n, &n_written, flags);
    if (s) return s;
    return n_written == tmp_n ? OSAL_SUCCESS : OSAL_STATUS_TIMEOUT;

}
#endif

osalStatus osal_stream_print_str(
    osalStream stream,
    const os_char *str,
    os_int flags)
{
    os_memsz n_written, str_sz;

    str_sz = os_strlen(str)-1;
    osalStatus s;

    s = osal_stream_write(stream, str, str_sz, &n_written, flags);
    if (s) return s;
    return n_written == str_sz ? OSAL_SUCCESS : OSAL_STATUS_TIMEOUT;
}

