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
#include "eosalx.h"
#if OSAL_SERIALIZE_SUPPORT

const os_char *osal_static_json_dict[OSAL_JSON_DICT_N_DEFINED]
 = {"-",        /* OSAL_JSON_DICT_NONE = 0, NONE must be "-" indicating array item */
    "groups",   /* OSAL_JSON_DICT_GROUPS = 1  */
    "signals",  /* OSAL_JSON_DICT_SIGNALS = 2  */
    "name",     /* OSAL_JSON_DICT_NAME = 3  */
    "value",    /* OSAL_JSON_DICT_VALUE = 4  */
    "type",     /* OSAL_JSON_DICT_TYPE = 5  */
    "addr",     /* OSAL_JSON_DICT_ADDR = 6  */
    "bank",     /* OSAL_JSON_DICT_BANK = 7  */
    "unit",     /* OSAL_JSON_DICT_UNIT = 8  */
    "min",      /* OSAL_JSON_DICT_MIN = 9 */
    "max",      /* OSAL_JSON_DICT_MAX = 10 */
    "digs",     /* OSAL_JSON_DICT_DIGS = 11 */
    OS_NULL,
    OS_NULL,
    "boolean",  /* OSAL_JSON_DICT_BOOLEAN = 14 */
    "char",     /* OSAL_JSON_DICT_CHAR = 15 */
    "uchar",    /* OSAL_JSON_DICT_UCHAR = 16 */
    "short",    /* OSAL_JSON_DICT_SHORT = 17 */
    "ushort",   /* OSAL_JSON_DICT_USHORT = 18 */
    "int",      /* OSAL_JSON_DICT_INT = 19 */
    "uint",     /* OSAL_JSON_DICT_UINT = 20 */
    "long",     /* OSAL_JSON_DICT_LONG = 21 */
    "float",    /* OSAL_JSON_DICT_FLOAT = 22 */
    "double",   /* OSAL_JSON_DICT_DOUBLE = 23 */
    "str",      /* OSAL_JSON_DICT_STR = 24 */
    OS_NULL,
    OS_NULL,
    "pins",     /* OSAL_JSON_DICT_PINS = 27 */
    "inputs",   /* OSAL_JSON_DICT_INPUTS = 28 */
    "outputs",  /* OSAL_JSON_DICT_OUTPUTS = 29 */
    "controls",   /* OSAL_JSON_DICT_CONTROLS = 30 */
    "frequency",  /* OSAL_JSON_DICT_FREQUENCY = 31 */
    "resolution"  /* OSAL_JSON_DICT_RESOLUTION = 32 */
   };

/* Find static dictionary item number by string, OSAL_JSON_DICT_NO_ENTRY is none.
 */
osalStaticJsonDictionary osal_find_in_static_json_dict(
    const os_char *str)
{
    os_int i;

    for (i = 0; i < OSAL_JSON_DICT_N_DEFINED; i++)
    {
        if (osal_static_json_dict[i] == OS_NULL) continue;
        if (*str != *osal_static_json_dict[i]) continue;
        if (!os_strcmp(str, osal_static_json_dict[i]))
        {
            return (osalStaticJsonDictionary)i;
        }
    }
    return OSAL_JSON_DICT_NO_ENTRY;
}


/* Get static dictionary string by item number, OS_NULL if none.
 */
const os_char *osal_get_static_json_dict_str(
    osalStaticJsonDictionary ix)
{
    if ((int)ix < 0 || ix >= OSAL_JSON_DICT_N_DEFINED) return OS_NULL;
    return osal_static_json_dict[ix];
}

#endif
