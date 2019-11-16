/**

  @file    osal_json_indexer.c
  @brief   Access compressed binary JSON data.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    28.10.2019

  Example, access compressed JSON data

      osalJsonIndex jindex;
      osalJsonItem item;

      s = osal_create_json_indexer(&jindex, ... )
      if (s) error...

      while (!(s = osal_get_json_item(&jindex, &item))
      {
        switch (item.code)
        {
            case OSAL_JSON_START_BLOCK:
                printf ("%s\n", item.tag_name);
                break;

            ...
        }
        ....
      }
      osal_release_json_indexer(&jindex);

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_SERIALIZE_SUPPORT


/**
****************************************************************************************************

  @brief Create indexer to access data in compressed JSON.
  @anchor osal_create_json_indexer

  The osal_create_json_indexer() function creates an indexer to access compressed json data
  from C code. The generated indexer should be released by calling osal_release_json_indexer()
  and the compressed data must remain in memory as long as the indexer is used.

  @param  jindex JSON data indexer to set up.
  @param  compressed Compressed binary data.
  @param  compressed_sz Size of compressed data in bytes.
  @param  flags Reserved for future, set 0 for now.
  @return OSAL_SUCCESS to indicate success. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus osal_create_json_indexer(
    osalJsonIndex *jindex,
    os_char *compressed,
    os_memsz compressed_sz,
    os_int flags)
{
    os_long dict_size, data_size;
    os_int bytes;
    os_ushort checksum;
    os_memsz sz_without_checksum;

    os_memclear(jindex, sizeof(osalJsonIndex));

    /* Must be at least four bytes. One for dictionary size, one byte for data
       size and two for checksum.
     */
    if (compressed_sz < sizeof(os_ushort) + 2) return OSAL_STATUS_FAILED;
    sz_without_checksum = compressed_sz - sizeof(os_short);
    os_memcpy(&checksum, compressed + sz_without_checksum, sizeof(os_short));
    if (checksum != os_checksum(compressed, sz_without_checksum, OS_NULL))
        return OSAL_CHECKSUM_ERROR;

    /* Calculate number of dictionary entries.
     */
    bytes = osal_intser_reader(compressed, &dict_size);
    jindex->dict_start = compressed + bytes;
    jindex->dict_end = jindex->dict_start + (os_memsz)dict_size;

    /* Set up data position and size. Start reading from beginning.
     */
    bytes = osal_intser_reader(jindex->dict_end, &data_size);
    jindex->data_start = jindex->dict_end + bytes;
    jindex->data_end = jindex->data_start + (os_memsz)data_size;
    jindex->read_pos = jindex->data_start;

    /* Verify that all pointers make sense
     */
    return (jindex->dict_start <= compressed ||
        jindex->dict_start > jindex->dict_end ||
        jindex->dict_end >= jindex->data_start ||
        jindex->data_start > jindex->data_end ||
        sz_without_checksum != jindex->data_end - compressed)
        ? OSAL_STATUS_FAILED
        : OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Create indexer to access data in compressed JSON.
  @anchor osal_create_json_indexer

  The osal_get_json_item() function gets information about one JSON item. This function can be
  called repeatedly after an indexer has been set up by osal_create_json_indexer(). This allows
  to loop trough all items in order they were in original JSON.

  @param  jindex JSON data indexer.
  @param  item Pointer to item information filled in by this function.
  @return OSAL_SUCCESS to indicate success or OSAL_END_OF_FILE to indicate that there is no
          more items. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus osal_get_json_item(
    osalJsonIndex *jindex,
    osalJsonItem *item)
{
    os_long code, m, e, l;
    os_int tag_dict_ix, value_dict_ix, bytes;
    os_float f;

    os_memclear(item, sizeof(osalJsonItem));
    if (jindex->read_pos >= jindex->data_end) return OSAL_END_OF_FILE;

    bytes = osal_intser_reader(jindex->read_pos, &code);
    jindex->read_pos += bytes;

    item->code = code & OSAL_JSON_CODE_MASK;

    item->depth = jindex->depth;

    if (item->code == OSAL_JSON_END_BLOCK ||
        item->code == OSAL_JSON_END_ARRAY)
    {
        jindex->depth--;
        item->depth--;
        return OSAL_SUCCESS;
    }

    tag_dict_ix = (os_int)(code >> OSAL_JSON_CODE_SHIFT);
    item->tag_name = osal_get_static_json_dict_str(tag_dict_ix);
    if (item->tag_name == OS_NULL)
    {
        if (tag_dict_ix < OSAL_JSON_DICT_N_STATIC)
        {
            return OSAL_STATUS_FAILED;
        }
        item->tag_name = jindex->dict_start + tag_dict_ix - OSAL_JSON_DICT_N_STATIC;

        if (item->tag_name < jindex->dict_start ||
            item->tag_name >= jindex->dict_end)
        {
            return OSAL_STATUS_FAILED;
        }
    }

    switch (code & OSAL_JSON_CODE_MASK)
    {
        case OSAL_JSON_START_BLOCK:
        case OSAL_JSON_START_ARRAY:
            jindex->depth++;
            break;

        case OSAL_JSON_VALUE_EMPTY:
            item->code = OSAL_JSON_VALUE_STRING;
            item->value.s = "";
            break;

        case OSAL_JSON_VALUE_STRING:
            bytes = osal_intser_reader(jindex->read_pos, &l);
            jindex->read_pos += bytes;
            value_dict_ix = (os_int)l;
            item->value.s = osal_get_static_json_dict_str(value_dict_ix);
            if (item->value.s == OS_NULL)
            {
                if (value_dict_ix < OSAL_JSON_DICT_N_STATIC)
                {
                    return OSAL_STATUS_FAILED;
                }
                item->value.s = jindex->dict_start + value_dict_ix - OSAL_JSON_DICT_N_STATIC;

                if (item->value.s < jindex->dict_start ||
                    item->value.s >= jindex->dict_end)
                {
                    return OSAL_STATUS_FAILED;
                }
            }
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
                osal_ints2float(&f, m, (os_short)e);
                item->value.d = f;
            }
            else
            {
                item->value.d = 0.0;
            }
            break;

        case OSAL_JSON_VALUE_TRUE:
        case OSAL_JSON_VALUE_FALSE:
            item->value.l = 0;
            break;

        case OSAL_JSON_VALUE_NULL:
            item->value.s = "";
            break;

        default:
            return OSAL_STATUS_FAILED;
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Release JSON index and any resources for it.
  @anchor osal_release_json_indexer

  The osal_release_json_indexer() function does nothing for now. It should be anyhow called, in
  case future versions of indexers allocate memory, etc ...

  @param  jindex JSON data index to release.
  @return none.

****************************************************************************************************
*/
void osal_release_json_indexer(
    osalJsonIndex *jindex)
{
}

#endif
