/**

  @file    osal_json_indexer.h
  @brief   Access compressed binary JSON data.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    28.10.2019

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_JSON_INDEXER_INCLUDED
#define OSAL_JSON_INDEXER_INCLUDED
#include "eosalx.h"
#if OSAL_SERIALIZE_SUPPORT


/** JSON index structure.
 */
typedef struct osalJsonIndex
{
    /** First byte of dictionary data and end (one byte past) of dictionary data.
     */
    os_char *dict_start;
    os_char *dict_end;

    /** First byte of compressed JSON items and end (one byte past) of item data.
     */
    os_char *data_start;
    os_char *data_end;

    /** Current read position within compressed data. Set to data_start by
        osal_create_json_indexer().
     */
    os_char *read_pos;

    /** Current recursion depth while processing compressed data.
     */
    os_int depth;
}
osalJsonIndex;


/** Information about single JSON item. The osal_get_json_item() function returns
    fills this structure.
 */
typedef struct osalJsonItem
{
    /** One of: OSAL_JSON_START_BLOCK, OSAL_JSON_END_BLOCK, OSAL_JSON_VALUE_STRING,
        OSAL_JSON_VALUE_INTEGER or OSAL_JSON_VALUE_FLOAT.
     */
    osalJsonElementCode code;

    /** Tag name is name in double quotes before colon. All codes except OSAL_JSON_END_BLOCK
        have a tag name set.
     */
    const os_char *tag_name;

    /** Reursion level in JSON. Starts from 0 at top level and grows when going deeper into
        JSON document os_int depth; All codes have depth.
     */
    os_int depth;

    /** Primitive value of the item, for codes OSAL_JSON_VALUE_STRING,
        OSAL_JSON_VALUE_INTEGER or OSAL_JSON_VALUE_FLOAT.
     */
    union
    {
        os_long l;          /* For code OSAL_JSON_VALUE_INTEGER */
        os_double d;        /* For OSAL_JSON_VALUE_FLOAT */
        const os_char *s;   /* For OSAL_JSON_VALUE_STRING */
    }
    value;
}
osalJsonItem;


/* Create indexer to access compressed data easily.
*/
osalStatus osal_create_json_indexer(
    osalJsonIndex *jindex,
    os_char *compressed,
    os_memsz compressed_sz,
    os_int flags);

/* Release indexer when no longer in use.
*/
void osal_release_json_indexer(
    osalJsonIndex *jindex);

/* Get next JSON item from the indexer.
 */
osalStatus osal_get_json_item(
    osalJsonIndex *jindex,
    osalJsonItem *item);

#endif
#endif
