/**

  @file    osal_compress_json.h
  @brief   Compress JSON as binary data.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    25.10.2019

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_COMPRESS_JSON_INCLUDED
#define OSAL_COMPRESS_JSON_INCLUDED
#include "eosalx.h"
#if OSAL_SERIALIZE_SUPPORT

/* Compress JSON from normal string presentation to binary format.
 */
osalStatus osal_compress_json(
    osalStream compressed,
    os_char *json_source,
    os_char *skip_tags,
    os_int flags);

#endif
#endif
