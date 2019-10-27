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

/** Enumeration of JSON element codes.
 */
typedef enum {
  OSAL_JSON_START_BLOCK = 1,
  OSAL_JSON_END_BLOCK = 2,
  OSAL_JSON_VALUE_EMPTY = 3,
  OSAL_JSON_VALUE_STRING = 4,
  OSAL_JSON_VALUE_INTEGER_ZERO = 5,
  OSAL_JSON_VALUE_INTEGER_ONE = 6,
  OSAL_JSON_VALUE_INTEGER = 7,
  OSAL_JSON_VALUE_FLOAT = 8
}
osalJsonElementCode;


/* Compress JSON from normal string presentation to binary format.
 */
osalStatus osal_compress_json(
    osalStream compressed,
    os_char *json_source);

#endif
#endif
