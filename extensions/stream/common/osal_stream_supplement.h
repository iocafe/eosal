/**

  @file    stream/commmon/osal_stream_supplement.h
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

#if OSAL_FUNCTION_POINTER_SUPPORT
#if OSAL_SERIALIZE_SUPPORT

osalStatus osal_stream_write_long(
    osalStream stream,
    os_long x,
    os_int flags);

#endif
#endif

#if OSAL_FUNCTION_POINTER_SUPPORT

osalStatus osal_stream_print_str(
    osalStream stream,
    const os_char *str,
    os_int flags);

#endif
