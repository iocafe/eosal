/**

  @file    osal_uncompress_json.c
  @brief   Uncompress JSON from binary data.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    25.10.2019

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

  The osal_uncompress_json() function uncompresses JSON from binary data to plain text string.

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
static osalStatus osal_write_json_str(
    osalStream uncompressed,
    const os_char *str);


/**
****************************************************************************************************

  @brief Create indexer to access data in compressed JSON.
  @anchor osal_create_json_indexer

  The osal_create_json_indexer() function creates an index to access compressed json data from
  C code. The generated indexer should be released by calling osal_release_json_indexer() and the
  compressed data must remain in memory as long as the indexer is used.

  @param  jindex JSON data index to set up.
  @param  compressed Compressed binary data.
  @param  compressed_sz Size of compressed data just for corruption check purposes.
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
    if (checksum != os_checksum((os_uchar*)compressed, sz_without_checksum, OS_NULL))
        return OSAL_CHECKSUM_ERROR;

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

    return sz_without_checksum == jindex->data_end - compressed ? OSAL_SUCCESS : OSAL_STATUS_FAILED;
}

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

    if (item->code == OSAL_JSON_END_BLOCK)
    {
        jindex->depth--;
        item->depth--;
        return /* jindex->depth < 0 ? OSAL_END_OF_FILE : */ OSAL_SUCCESS;
    }

    tag_dict_ix = (os_int)(code >> OSAL_JSON_CODE_SHIFT);
    /* if (tag_dict_ix < 0 || tag_dict_ix >= jindex->dictionary_n)
    {
        return OSAL_STATUS_FAILED;
    } */

    item->tag_name = osal_get_static_json_dict_str(tag_dict_ix);
    if (item->tag_name == OS_NULL)
    {
        if (tag_dict_ix < OSAL_JSON_DICT_N_STATIC /* ||
            tag_dict_ix > jindex->dictionary_n + OSAL_JSON_DICT_N_STATIC */)
        {
            return OSAL_STATUS_FAILED;
        }
        item->tag_name = jindex->dict_start + tag_dict_ix - OSAL_JSON_DICT_N_STATIC;
    }

    switch (code & OSAL_JSON_CODE_MASK)
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
            value_dict_ix = (os_int)l;
            // if (value_dict_ix < 0 || value_dict_ix >= jindex->dictionary_n) return OSAL_STATUS_FAILED;

            item->value.s = osal_get_static_json_dict_str(value_dict_ix);
            if (item->value.s == OS_NULL)
            {
                if (value_dict_ix < OSAL_JSON_DICT_N_STATIC) return OSAL_STATUS_FAILED;
                item->value.s = jindex->dict_start + value_dict_ix - OSAL_JSON_DICT_N_STATIC;
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
                osal_ints2float(&f, m, e);
                item->value.d = f;
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


/**
****************************************************************************************************

  @brief Uncompress JSON from binary data to text.
  @anchor osal_uncompress_json

  The osal_uncompress_json() function compresses JSON strin to packed binary format.

  @param  ucompressed Stream where to write uncompressed JSON output.
  @param  compressed Compressed binary data.
  @param  compressed_sz Size of compressed data just for corruption check purposes.
  @param  flags Reserved for future, set 0 for now.
  @return OSAL_SUCCESS to indicate success. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus osal_uncompress_json(
    osalStream uncompressed,
    os_char *compressed,
    os_memsz compressed_sz,
    os_int flags)
{
    osalJsonIndex jindex;
    osalJsonItem  item;
    osalStatus s;
    os_int i, prev_depth, ddigs;
    os_char nbuf[OSAL_NBUF_SZ];
    os_double d;
    os_memsz n_written;

    s = osal_create_json_indexer(&jindex, compressed, compressed_sz, 0);
    if (s) return s;

    prev_depth = -1;

    while (!(s = osal_get_json_item(&jindex, &item)))
    {
        if (item.depth == prev_depth)
        {
            s = osal_write_json_str(uncompressed, ",");
            if (s) goto getout;
        }
        prev_depth = item.depth;

        s = osal_write_json_str(uncompressed, "\n");
        if (s) goto getout;

        for (i = 0; i<item.depth; i++)
        {
            s = osal_write_json_str(uncompressed, "\t");
            if (s) goto getout;
        }

        if (item.code == OSAL_JSON_END_BLOCK)
        {
            s = osal_write_json_str(uncompressed, "}");
            if (s) goto getout;
            continue;
        }

        s = osal_write_json_str(uncompressed, item.tag_name);
        if (s) goto getout;

        switch (item.code)
        {
            case OSAL_JSON_START_BLOCK:
                s = osal_write_json_str(uncompressed, ": {");
                if (s) goto getout;
                break;

            case OSAL_JSON_VALUE_STRING:
                osal_int_to_string(nbuf, sizeof(nbuf), item.value.l);
                s = osal_write_json_str(uncompressed, ": \"");
                if (s) goto getout;
                s = osal_write_json_str(uncompressed, item.value.s);
                if (s) goto getout;
                s = osal_write_json_str(uncompressed, "\"");
                if (s) goto getout;
                break;

            case OSAL_JSON_VALUE_INTEGER:
                osal_int_to_string(nbuf, sizeof(nbuf), item.value.l);
                s = osal_write_json_str(uncompressed, ": ");
                if (s) goto getout;
                s = osal_write_json_str(uncompressed, nbuf);
                if (s) goto getout;
                break;

            case OSAL_JSON_VALUE_FLOAT:
                d = item.value.d; if (d < 0) d = -d;
                ddigs = 5; while (d >= 1.0 && ddigs > 1) {ddigs--; d *= 0.1; }
                osal_double_to_string(nbuf, sizeof(nbuf),
                    item.value.d, ddigs, OSAL_FLOAT_DEFAULT);
                s = osal_write_json_str(uncompressed, ": ");
                if (s) goto getout;
                s = osal_write_json_str(uncompressed, nbuf);
                if (s) goto getout;
                break;

            default:
                s = OSAL_STATUS_FAILED;
                goto getout;
        }
    }

    if (s == OSAL_END_OF_FILE)
    {
        /* Write terminating '\n' and '\0' characters.
         */
        s = osal_stream_write(uncompressed, (os_uchar*)"\n", 2,
            &n_written, OSAL_STREAM_DEFAULT);
    }

getout:
    osal_release_json_indexer(&jindex);
    return s;
}

static osalStatus osal_write_json_str(
    osalStream uncompressed,
    const os_char *str)
{
    os_memsz n_written;

    return osal_stream_write(uncompressed, (os_uchar*)str, os_strlen(str)-1,
        &n_written, OSAL_STREAM_DEFAULT);
}


#endif
