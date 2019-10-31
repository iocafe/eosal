/**

  @file    osal_uncompress_json.c
  @brief   Uncompress JSON from binary data.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    25.10.2019

  The osal_uncompress_json() function uncompresses JSON from binary data to plain text string.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_JSON_TEXT_SUPPORT

/* Forward referred static functions.
 */
static osalStatus osal_write_json_str(
    osalStream uncompressed,
    const os_char *str);


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
    os_boolean is_array_item;

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

        if (item.code == OSAL_JSON_END_ARRAY)
        {
            s = osal_write_json_str(uncompressed, "]");
            if (s) goto getout;
            continue;
        }

        is_array_item = (os_boolean)!os_strcmp(item.tag_name, "-");
        if (!is_array_item)
        {
            s = osal_write_json_str(uncompressed, "\"");
            if (s) goto getout;
            s = osal_write_json_str(uncompressed, item.tag_name);
            if (s) goto getout;
            s = osal_write_json_str(uncompressed, "\"");
            if (s) goto getout;
        }

        switch (item.code)
        {
            case OSAL_JSON_START_BLOCK:
                s = osal_write_json_str(uncompressed, is_array_item ? "{" : " : {");
                if (s) goto getout;
                break;

            case OSAL_JSON_START_ARRAY:
                s = osal_write_json_str(uncompressed, ": [");
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
        s = osal_stream_write(uncompressed, "\n", 2,
            &n_written, OSAL_STREAM_DEFAULT);
    }

getout:
    osal_release_json_indexer(&jindex);
    return s;
}


/**
****************************************************************************************************

  @brief Helper string to write a string to uncompressed stream.
  @anchor osal_write_json_str

  The osal_write_json_str() function does nothing for now. It should be anyhow called, in
  case future versions of indexers allocate memory, etc ...

  @param   uncompressed Stream to write to.
  @param   str NULL terminated string to write.
  @return  none.

****************************************************************************************************
*/
static osalStatus osal_write_json_str(
    osalStream uncompressed,
    const os_char *str)
{
    os_memsz sz, n_written;
    osalStatus s;

    sz = os_strlen(str) - 1;
    s = osal_stream_write(uncompressed, str, sz,
        &n_written, OSAL_STREAM_DEFAULT);
    if (s) return s;

    return n_written == sz ? OSAL_SUCCESS : OSAL_STATUS_TIMEOUT;
}


#endif
