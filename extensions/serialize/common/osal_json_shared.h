/**

  @file    osal_json_shared.h
  @brief   Shared defines for compressing and uncompressing JSON.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    25.10.2019

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_JSON_SHARED_INCLUDED
#define OSAL_JSON_INCLUDED
#include "eosalx.h"
#if OSAL_SERIALIZE_SUPPORT

/** Enumeration of JSON element codes.
 *  Range 0..7 if OSAL_JSON_CODE_SHIFT is 3.
 */
typedef enum osalJsonElementCode
{
  OSAL_JSON_START_BLOCK = 0,
  OSAL_JSON_END_BLOCK = 1,
  OSAL_JSON_VALUE_EMPTY = 2,
  OSAL_JSON_VALUE_STRING = 3,
  OSAL_JSON_VALUE_INTEGER_ZERO = 4,
  OSAL_JSON_VALUE_INTEGER_ONE = 5, /* this can be taken if needed for future change */
  OSAL_JSON_VALUE_INTEGER = 6,
  OSAL_JSON_VALUE_FLOAT = 7
}
osalJsonElementCode;

/* How much to shive tag dictionary index left to make space for element code.
 */
#define OSAL_JSON_CODE_SHIFT 3

/* Mask to get only code. Needs to match with OSAL_JSON_CODE_SHIFT.
 */
#define OSAL_JSON_CODE_MASK 7

/* Enumeration of static dictionary entries.
 */
typedef enum osalStaticJsonDictionary
{
    OSAL_JSON_DICT_ADDR = 0,
    OSAL_JSON_DICT_BANK = 1,

    OSAL_JSON_DICT_NAME = 3,
    OSAL_JSON_DICT_VALUE = 4,
    OSAL_JSON_DICT_TYPE = 5,
    OSAL_JSON_DICT_UNIT = 6,
    OSAL_JSON_DICT_MIN = 7,
    OSAL_JSON_DICT_MAX = 8,
    OSAL_JSON_DICT_DIGS = 9,

    OSAL_JSON_DICT_FREQUENCY = 12,
    OSAL_JSON_DICT_RESOLUTION = 13,
    OSAL_JSON_DICT_DELAY = 14,

    OSAL_JSON_DICT_NO_ENTRY = 20,
    OSAL_JSON_DICT_N_STATIC = OSAL_JSON_DICT_NO_ENTRY
}
osalStaticJsonDictionary;

/* Find static dictionary item number by string, OSAL_JSON_DICT_NO_ENTRY is none.
 */
osalStaticJsonDictionary osal_find_in_static_json_dict(
    const os_char *str);

/* Get static dictionary string by item number, OS_NULL if none.
 */
const os_char *osal_get_static_json_dict_str(
    osalStaticJsonDictionary ix);

#endif
#endif
