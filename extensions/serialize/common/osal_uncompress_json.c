/**

  @file    osal_uncompress_json.c
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
#include "eosalx.h"
#if OSAL_SERIALIZE_SUPPORT

/* Forward referred static functions.
 */
#include <stdio.h>


/**
****************************************************************************************************

  @brief Create index to access compressed data easily.
  @anchor osal_create_json_index

  The osal_create_json_index() function creates an index to access compressed json data from
  C code. The generated index must be released by calling osal_release_json_index() and the
  compressed data must remain in memory as long as index is used.

  @param  jindex JSON data index to set up.
  @param  compressed Compressed binary data.
  @param  compressed_sz Size of compressed data just for corruption check purposes.
  @return OSAL_SUCCESS to indicate success. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus osal_create_json_index(
    osalJsonIndex *jindex,
    os_char *compressed,
    os_memsz compressed_sz)
{
    os_long dict_size, data_size;
    os_int bytes;

    os_memclear(jindex, sizeof(osalJsonIndex));
    jindex->compressed = compressed;
    jindex->compressed_sz = compressed_sz;

    /* Calculate number of dictionary entries.
     */
    bytes = osal_intser_reader(compressed, &dict_size);
    if (bytes < 1 || dict_size < 0 || dict_size >= compressed_sz) return OSAL_STATUS_FAILED;
    jindex->dict_start = compressed + bytes;
    jindex->dict_end = jindex->dict_start + (os_memsz)dict_size;

    /* Set up data position and size. Start reading from beginning.
     */
    bytes = osal_intser_reader(jindex->dict_end, &data_size);
    jindex->data_start = jindex->dict_end + bytes;
    jindex->data_end = jindex->data_start + (os_memsz)data_size;
    jindex->read_pos = jindex->data_start;

    return compressed_sz == jindex->data_end - compressed ? OSAL_SUCCESS : OSAL_STATUS_FAILED;
}

osalStatus osal_get_json_item(
    osalJsonIndex *jindex,
    osalJsonItem *item)
{
    os_long code, m, e, l;
    os_int tag_dict_ix, value_dict_ix, bytes;

    os_memclear(item, sizeof(osalJsonItem));
    bytes = osal_intser_reader(jindex->read_pos, &code);
    jindex->read_pos += bytes;

    item->code = code & 15;

    item->depth = jindex->depth;

    if (item->code == OSAL_JSON_END_BLOCK)
    {
        jindex->depth--;
        item->depth--;
        return OSAL_SUCCESS;
    }

    tag_dict_ix = code >> 4;
//    if (tag_dict_ix < 0 || tag_dict_ix >= jindex->dictionary_n) return OSAL_STATUS_FAILED;

    item->tag_name = jindex->dict_start + tag_dict_ix;

    switch (code & 15)
    {
        case OSAL_JSON_START_BLOCK:
            jindex->depth++;
            break;

        case OSAL_JSON_VALUE_EMPTY:
            item->code = OSAL_JSON_VALUE_STRING;
            item->value.s = "";
            break;

        case OSAL_JSON_VALUE_STRING:
            bytes = osal_intser_reader(jindex->read_pos, &l);
            jindex->read_pos += bytes;
            value_dict_ix = l;
            // if (value_dict_ix < 0 || value_dict_ix >= jindex->dictionary_n) return OSAL_STATUS_FAILED;
            item->value.s = jindex->dict_start + value_dict_ix;
            break;

        case OSAL_JSON_VALUE_INTEGER_ZERO:
            item->code = OSAL_JSON_VALUE_INTEGER;
            item->value.l = 0;
            break;

        case OSAL_JSON_VALUE_INTEGER_ONE:
            item->code = OSAL_JSON_VALUE_INTEGER;
            item->value.l = 1;
            break;

        case OSAL_JSON_VALUE_INTEGER:
            bytes = osal_intser_reader(jindex->read_pos, &item->value.l);
            jindex->read_pos += bytes;
            break;

        case OSAL_JSON_VALUE_FLOAT:
            bytes = osal_intser_reader(jindex->read_pos, &m);
            jindex->read_pos += bytes;
            if (m)
            {
                bytes = osal_intser_reader(jindex->read_pos, &e);
                jindex->read_pos += bytes;
                osal_ints2double(&item->value.d, m, e);
            }
            else
            {
                item->value.d = 0.0;
            }
            break;

        default:
            return OSAL_STATUS_FAILED;
    }

    return OSAL_SUCCESS;
}



/**
****************************************************************************************************

  @brief Release JSON index and memory allocated for it.
  @anchor osal_create_json_index

  The osal_create_json_index() function creates an index to access compressed json data from
  C code. The inde

  @param  jindex JSON data index to release.
  @return none.

****************************************************************************************************
*/
void osal_release_json_index(
    osalJsonIndex *jindex)
{
}


/**
****************************************************************************************************

  @brief Uncompress JSON from binary data to text.
  @anchor osal_compress_json

  The osal_uncompress_json() function compresses JSON strin to packed binary format.

  @param  ucompressed Stream where to write uncompressed JSON output.
  @param  compressed Compressed binary data.
  @param  compressed_sz Size of compressed data just for corruption check purposes.
  @return OSAL_SUCCESS to indicate success. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus osal_uncompress_json(
    osalStream uncompressed,
    os_char *compressed,
    os_memsz compressed_sz)
{
    osalJsonIndex jindex;
    osalJsonItem  item;
    osalStatus s;
    os_int i, prev_depth;

    s = osal_create_json_index(&jindex, compressed, compressed_sz);
    if (s) return s;

    prev_depth = -1
    ;
    while (!osal_get_json_item(&jindex, &item))
    {
        if (item.depth == prev_depth)
        {
            printf (",");
        }
        prev_depth = item.depth;

        printf ("\n");
        for (i = 0; i<item.depth; i++) printf (" \t");

        if (item.code == OSAL_JSON_END_BLOCK)
        {
            printf ("}");
            continue;
        }

        printf ("%s", item.tag_name);

        switch (item.code)
        {
            case OSAL_JSON_START_BLOCK:
                printf (": {");
                break;

            case OSAL_JSON_VALUE_STRING:
                printf (": \"%s\"", item.value.s);
                break;

            case OSAL_JSON_VALUE_INTEGER:
                printf (": %lld", item.value.l);
                break;

            case OSAL_JSON_VALUE_FLOAT:
                printf (": %f", item.value.d);
                break;

            default:
                printf (": \"error\"");
                break;

        }
    }

    osal_release_json_index(&jindex);
    return OSAL_SUCCESS;
}

#endif
