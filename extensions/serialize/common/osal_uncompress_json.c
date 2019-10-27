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
    os_long dict_size;
    os_int bytes, dictionary_n, ix, dict_start, pos, dict_end;
    os_long len;

    os_memclear(jindex, sizeof(osalJsonIndex));
    jindex->compressed = compressed;
    jindex->compressed_sz = compressed_sz;

    /* Calculate number of dictionary entries.
     */
    bytes = osal_intser_reader(compressed, &dict_size);
    if (bytes < 1 || dict_size < 0 || dict_size >= compressed_sz) return OSAL_STATUS_FAILED;
    dict_start = bytes;

    /* Calculate number of dictionary entries.
     */
    pos = dict_start;
    dictionary_n = 0;
    dict_end = dict_start + dict_size;
    while (pos < dict_end)
    {
        dictionary_n++;
        bytes = osal_intser_reader(compressed + pos, &len);
        pos += bytes + len;
        if (bytes + len < 1 || pos > dict_end) return OSAL_STATUS_FAILED;
    }

    /* Allocate dictionary
     */
    jindex->dictionary = (osal_json_pos_t*)os_malloc(dictionary_n * sizeof(osal_json_pos_t), OS_NULL);
    jindex->dictionary_n = dictionary_n;

    /* Fill in dictionary string pointers.
     */
    pos = dict_start;
    ix = 0;
    while (pos < dict_end)
    {
        dictionary_n++;
        bytes = osal_intser_reader(compressed + pos, &len);
        jindex->dictionary[ix] = (osal_json_pos_t)(pos + bytes);
        pos += bytes + len;
        ix++;
    }

    /* Set up data position and size. Start reading from beginning.
     */
    bytes = osal_intser_reader(compressed + dict_end, &len);
    jindex->data_start = dict_end + bytes;
    jindex->data_size = len;
    jindex->read_pos = jindex->data_start;

    return OSAL_SUCCESS;
}

osalStatus osal_get_json_item(
    osalJsonIndex *jindex,
    osalJsonItem *item)
{
    os_long code, m, e, l;
    os_int tag_dict_ix, value_dict_ix, bytes;
    os_char *compressed;

    compressed = jindex->compressed;
    os_memclear(item, sizeof(osalJsonItem));

    bytes = osal_intser_reader(compressed + jindex->read_pos, &code);
    jindex->read_pos += bytes;

    item->code = code & 15;

    if (item->code == OSAL_JSON_END_BLOCK)
    {
        jindex->depth--;
        return OSAL_SUCCESS;
    }

    tag_dict_ix = code >> 4;
    if (tag_dict_ix < 0 || tag_dict_ix >= jindex->dictionary_n) return OSAL_STATUS_FAILED;

    item->tag_name = compressed + jindex->dictionary[tag_dict_ix];

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
            bytes = osal_intser_reader(compressed + jindex->read_pos, &l);
            jindex->read_pos += bytes;
            value_dict_ix = l;
            if (value_dict_ix < 0 || value_dict_ix >= jindex->dictionary_n) return OSAL_STATUS_FAILED;
            item->value.s = compressed + jindex->dictionary[value_dict_ix];
            break;

        case OSAL_JSON_VALUE_INTEGER_ZERO:
            item->code = OSAL_JSON_VALUE_INTEGER;
            item->value.l = 0;
            break;

        case OSAL_JSON_VALUE_INTEGER_ONE:
            item->code = OSAL_JSON_VALUE_INTEGER;
            item->value.l = 1;
            break;

        case OSAL_JSON_VALUE_FLOAT:
            bytes = osal_intser_reader(compressed + jindex->read_pos, &m);
            jindex->read_pos += bytes;
            if (m)
            {
                bytes = osal_intser_reader(compressed + jindex->read_pos, &e);
                jindex->read_pos += bytes;

                // make_doub;
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
    os_free(jindex->dictionary, jindex->dictionary_n * sizeof(osal_json_pos_t));
    os_memclear(jindex, sizeof(osalJsonIndex));
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

    s = osal_create_json_index(&jindex, compressed, compressed_sz);
    if (s) return s;


    while (!osal_get_json_item(&jindex, &item))
    {
        printf ("taggi %s\n", item.tag_name);
    }


    osal_release_json_index(&jindex);
    return OSAL_SUCCESS;
}

#endif
