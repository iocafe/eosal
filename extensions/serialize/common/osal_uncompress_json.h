/**

  @file    osal_uncompress_json.h
  @brief   Uncompress JSON from binary data.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    25.10.2019

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_UNCOMPRESS_JSON_INCLUDED
#define OSAL_UNCOMPRESS_JSON_INCLUDED
#include "eosalx.h"
#if OSAL_SERIALIZE_SUPPORT

/* We can use either 32 bit or 64 bit type as position index. 16 bit limits maximum
   compressed JSON size to 64k for indexing. This doesn't effect compressed binary format.
 */
typedef os_ushort osal_json_pos_t;


/** JSON index structure.
 */
typedef struct osalJsonIndex
{
    os_char *compressed;
    os_memsz compressed_sz;

    /* Dictionary of strings.
     */
    osal_json_pos_t *dictionary;

    /* Number of items in dictionary
     */
    os_int dictionary_n;

    os_int data_start;
    os_int data_size;

    os_int read_pos;
    os_int depth;
}
osalJsonIndex;


typedef struct osalJsonItem
{
    os_char *tag_name;

    osalJsonElementCode code;

    os_int depth;

    union
    {
        os_long l;
        os_double d;
        os_char *s;
    }
    value;
}
osalJsonItem;


/* Create index to access compressed data easily.
*/
osalStatus osal_create_json_index(
    osalJsonIndex *jindex,
    os_char *compressed,
    os_memsz compressed_sz);

/* Release JSON index and memory allocated for it.
*/
void osal_release_json_index(
    osalJsonIndex *jindex);

/* Uncompress JSON from binary data to text.
 */
osalStatus osal_uncompress_json(
    osalStream uncompressed,
    os_char *compressed,
    os_memsz compressed_sz);

#endif
#endif
