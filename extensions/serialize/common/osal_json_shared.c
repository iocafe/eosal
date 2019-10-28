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
 = {"addr",  /* OSAL_JSON_DICT_ADDR = 0 */
    "bank",  /* OSAL_JSON_DICT_BANK = 1 */
    OS_NULL,
    "name",  /* OSAL_JSON_DICT_NAME = 3 */
    "value", /* OSAL_JSON_DICT_VALUE = 4 */
    "type",  /* OSAL_JSON_DICT_TYPE = 5 */
    "unit",  /* OSAL_JSON_DICT_UNIT = 6 */
    "min",   /* OSAL_JSON_DICT_MIN = 7 */
    "max",   /* OSAL_JSON_DICT_MAX = 8 */
    "digs",  /* OSAL_JSON_DICT_DIGS = 9 */
    OS_NULL,
    OS_NULL,
    "frequency",  /* OSAL_JSON_DICT_FREQUENCY = 12 */
    "resolution", /* OSAL_JSON_DICT_RESOLUTION = 13 */
    "delay"       /* OSAL_JSON_DICT_DELAY = 14 */
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
    if (ix < 0 || ix >= OSAL_JSON_DICT_N_DEFINED) return OS_NULL;
    return osal_static_json_dict[ix];
}

#endif
