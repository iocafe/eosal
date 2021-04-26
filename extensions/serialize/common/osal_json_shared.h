/**

  @file    osal_json_shared.h
  @brief   Shared defines for compressing and uncompressing JSON.
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
#ifndef OSAL_JSON_SHARED_H_
#define OSAL_JSON_SHARED_H_
#include "eosalx.h"

#if OSAL_SERIALIZE_SUPPORT

/** Enumeration of JSON element codes.
 *  Range 0..7 if OSAL_JSON_CODE_SHIFT is 4.
 */
typedef enum osalJsonElementCode
{
  OSAL_JSON_START_BLOCK = 0,
  OSAL_JSON_END_BLOCK = 1,
  OSAL_JSON_VALUE_EMPTY = 2,
  OSAL_JSON_VALUE_STRING = 3,
  OSAL_JSON_VALUE_INTEGER_ZERO = 4,
  OSAL_JSON_VALUE_INTEGER_ONE = 5,
  OSAL_JSON_VALUE_INTEGER = 6,
  OSAL_JSON_VALUE_FLOAT = 7,
  OSAL_JSON_START_ARRAY = 8,
  OSAL_JSON_END_ARRAY = 9,
  OSAL_JSON_VALUE_NULL = 10,
  OSAL_JSON_VALUE_TRUE = 11,
  OSAL_JSON_VALUE_FALSE = 12
}
osalJsonElementCode;

/* How much to shive tag dictionary index left to make space for element code.
 */
#define OSAL_JSON_CODE_SHIFT 4

/* Mask to get only code. Needs to match with OSAL_JSON_CODE_SHIFT.
 */
#define OSAL_JSON_CODE_MASK 15

/* Enumeration of static dictionary entries.
 */
typedef enum osalStaticJsonDictionary
{
    OSAL_JSON_DICT_NONE = 0,
    OSAL_JSON_DICT_GROUPS = 1,
    OSAL_JSON_DICT_SIGNALS = 2,
    OSAL_JSON_DICT_NAME = 3,
    OSAL_JSON_DICT_ARRAY = 4,
    OSAL_JSON_DICT_TYPE = 5,
    OSAL_JSON_DICT_ADDR = 6,
    OSAL_JSON_DICT_BANK = 7,
    OSAL_JSON_DICT_UNIT = 8,
    OSAL_JSON_DICT_MIN = 9,
    OSAL_JSON_DICT_MAX = 10,
    OSAL_JSON_DICT_DIGS = 11,
    OSAL_JSON_DICT_MBLK = 12,
    OSAL_JSON_DICT_PFLAG = 13,
    OSAL_JSON_DICT_BOOLEAN = 14,
    OSAL_JSON_DICT_CHAR = 15,
    OSAL_JSON_DICT_UCHAR = 16,
    OSAL_JSON_DICT_SHORT = 17,
    OSAL_JSON_DICT_USHORT = 18,
    OSAL_JSON_DICT_INT = 19,
    OSAL_JSON_DICT_UINT = 20,
    OSAL_JSON_DICT_LONG = 21,
    OSAL_JSON_DICT_FLOAT = 22,
    OSAL_JSON_DICT_DOUBLE = 23,
    OSAL_JSON_DICT_STR = 24,
    OSAL_JSON_DICT_EXP = 25,
    OSAL_JSON_DICT_IMP = 26,
    OSAL_JSON_DICT_ASTERISK = 27,
    OSAL_JSON_DICT_NETWORK = 28,
    OSAL_JSON_DICT_PUBLISH = 29,
    OSAL_JSON_DICT_CONNECT = 30,
    OSAL_JSON_DICT_FLAGS = 31,
    OSAL_JSON_DICT_TRANSPORT = 32,
    OSAL_JSON_DICT_PARAMETERS = 33,
    OSAL_JSON_DICT_DEVICE_NR = 34,
    OSAL_JSON_DICT_NETWORK_NAME = 35,
    OSAL_JSON_DICT_PASSWORDE = 36,
    OSAL_JSON_DICT_GATEWAY = 37,
    OSAL_JSON_DICT_SUBNET = 38,

    OSAL_JSON_DICT_N_DEFINED,
    OSAL_JSON_DICT_NO_ENTRY = 40,
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
