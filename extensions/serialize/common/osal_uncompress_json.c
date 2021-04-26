/**

  @file    osal_uncompress_json.c
  @brief   Uncompress JSON from binary data.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  The osal_uncompress_json() function uncompresses JSON from binary data to plain text string.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
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

static osalStatus osal_write_escaped_json_str(
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
    os_char nbuf[OSAL_NBUF_SZ], *p;
    os_double d;
    os_boolean is_array_item;

    s = osal_write_json_str(uncompressed, "{");
    if (s) return s;

    s = osal_create_json_indexer(&jindex, compressed, compressed_sz, 0);
    if (s) return s;

    prev_depth = -1;

    while (!(s = osal_get_json_item(&jindex, &item)))
    {
        if (item.depth == prev_depth)
        {
            if (item.code != OSAL_JSON_END_ARRAY &&
                item.code != OSAL_JSON_END_BLOCK)
            {
                s = osal_write_json_str(uncompressed, ",");
                if (s) goto getout;
            }
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
                s = osal_write_json_str(uncompressed, ": \"");
                if (s) goto getout;
                s = osal_write_escaped_json_str(uncompressed, item.value.s);
                if (s) goto getout;
                s = osal_write_json_str(uncompressed, "\"");
                if (s) goto getout;
                break;

            case OSAL_JSON_VALUE_INTEGER:
                osal_int_to_str(nbuf, sizeof(nbuf), item.value.l);
                s = osal_write_json_str(uncompressed, ": ");
                if (s) goto getout;
                s = osal_write_json_str(uncompressed, nbuf);
                if (s) goto getout;
                break;

            case OSAL_JSON_VALUE_FLOAT:
                d = item.value.d; if (d < 0) d = -d;
                ddigs = 5; while (d >= 1.0 && ddigs > 1) {ddigs--; d *= 0.1; }
                osal_double_to_str(nbuf, sizeof(nbuf),
                    item.value.d, ddigs, OSAL_FLOAT_DEFAULT);
                s = osal_write_json_str(uncompressed, ": ");
                if (s) goto getout;
                s = osal_write_json_str(uncompressed, nbuf);
                if (s) goto getout;
                break;

            /* Handling OSAL_JSON_VALUE_NULL, OSAL_JSON_VALUE_TRUE, and
               OSAL_JSON_VALUE_FALSE is needed only if compressed with
               OSAL_JSON_KEEP_QUIRKS flag.
             */
            case OSAL_JSON_VALUE_NULL:
                p = "null";
                goto print_quirk;

            case OSAL_JSON_VALUE_TRUE:
                p = "true";
                goto print_quirk;

            case OSAL_JSON_VALUE_FALSE:
                p = "false";
print_quirk:
                s = osal_write_json_str(uncompressed, ": ");
                if (s) goto getout;
                s = osal_write_json_str(uncompressed, p);
                if (s) goto getout;
                break;

            default:
                s = OSAL_STATUS_FAILED;
                goto getout;
        }
    }

    if (s == OSAL_END_OF_FILE)
    {
        /* Write terminating '\n' and '}' characters.
         */
        s = osal_write_json_str(uncompressed, "\n}\n");
    }

getout:
    osal_release_json_indexer(&jindex);
    return s;
}


/**
****************************************************************************************************

  @brief Helper string to write a string to uncompressed stream.
  @anchor osal_write_json_str

  The osal_write_json_str() function writes NULL terminated string to stream.

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


/**
****************************************************************************************************

  @brief Helper string to write a string with escape marks, like \n.
  @anchor osal_write_escaped_json_str

  The osal_write_escaped_json_str() function extends escapes in string and writes it
  to stream.

  @param   uncompressed Stream to write to.
  @param   str NULL terminated string to write.
  @return  none.

****************************************************************************************************
*/
static osalStatus osal_write_escaped_json_str(
    osalStream uncompressed,
    const os_char *str)
{
    os_char escape_c, escape_str[3];
    const os_char *p;
    os_memsz sz, n_written;
    osalStatus s;

    p = str;
    while (*p)
    {
        switch (*p)
        {
            /* case '/': We do not want to escape this, would break paths */
            case '\"':
            case '\\': escape_c = *p; break;
            case '\n': escape_c = 'n'; break;
            case '\r': escape_c = 'r'; break;
            case '\b': escape_c = 'b'; break;
            case '\f': escape_c = 'f'; break;
            case '\t': escape_c = 't'; break;
            default: escape_c = 0; p++; break;
        }

        if (escape_c)
        {
            if (p > str)
            {
                sz = p - str;
                s = osal_stream_write(uncompressed, str, sz,
                    &n_written, OSAL_STREAM_DEFAULT);
                if (s) return s;
                if (n_written != sz) return OSAL_STATUS_TIMEOUT;
                str = ++p;
            }

            escape_str[0] = '\\';
            escape_str[1] = escape_c;
            escape_str[2] = '\0';

            s = osal_write_json_str(uncompressed, escape_str);
            if (s) return s;
        }
    }

    if (p > str)
    {
        s = osal_write_json_str(uncompressed, str);
        if (s) return s;
    }
    return OSAL_SUCCESS;
}

#endif
