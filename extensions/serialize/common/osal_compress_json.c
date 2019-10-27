/**

  @file    osal_compress_json.c
  @brief   Compress JSON as binary data.
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


/** JSON compression state structure.
 */
typedef struct
{
    /* Current position in JSON source.
     */
    os_char *pos;

    /* Buffer for string being parsed
     */
    osalStream str;

    /* Buffer for content being created.
     */
    osalStream content;

    /* Buffer for dictionary.
     */
    osalStream dictionary;

    /* Block recursion depth.
     */
  //  os_int depth;
}
osalJsonCompressor;


/* Forward referred static functions.
 */
static osalStatus osal_parse_json_recursive(
    osalJsonCompressor *state);

static osalStatus osal_parse_json_tag(
    osalJsonCompressor *state,
    os_long *dict_ix);

static osalStatus parse_json_value(
    osalJsonCompressor *state,
    os_long tag_dict_ix);

static osalStatus osal_parse_json_quoted_string(
    osalJsonCompressor *state);

static osalStatus osal_parse_json_number(
    osalJsonCompressor *state);

static os_long osal_add_string_to_json_dict(
    osalJsonCompressor *state);


/**
****************************************************************************************************

  @brief Compress JSON from normal string presentation to binary format.
  @anchor osal_compress_json

  The osal_compress_json() function compresses JSON strin to packed binary format.

  @param  compressed Stream where to write compressed output.
  @param  x Integer to pack.
  @return OSAL_SUCCESS to indicate success. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus osal_compress_json(
    osalStream compressed,
    os_char *json_source)
{
    osalJsonCompressor state;
    os_char c;
    os_uchar *data;
    os_memsz data_sz, n_written;
    osalStatus s = OSAL_STATUS_FAILED;

    os_memclear(&state, sizeof(state));
    state.pos = json_source;

    /* Allocate temporary buffers for parsing string, generating content and for dictionary.
     */
    state.str = osal_stream_buffer_open(OS_NULL, OS_NULL, OS_NULL, OSAL_STREAM_DEFAULT);
    state.content = osal_stream_buffer_open(OS_NULL, OS_NULL, OS_NULL, OSAL_STREAM_DEFAULT);
    state.dictionary = osal_stream_buffer_open(OS_NULL, OS_NULL, OS_NULL, OSAL_STREAM_DEFAULT);
    if (state.str == OS_NULL || state.content == OS_NULL || state.dictionary == OS_NULL)
    {
        goto getout;
    }

    /* Skip the first '{'
     */
    do
    {
        c = *(state.pos++);
        if (c == '\0') goto getout;
    }
    while (osal_char_isspace(c));
    if (c != '{') goto getout;

    /* Parse the JSON recursively.
     */
    s = osal_parse_json_recursive(&state);
    if (s) goto getout;

    /* Write dictionary size and the dictionary.
     */
    data = osal_stream_buffer_content(state.dictionary, &data_sz);
    s = osal_stream_write_long(compressed, data_sz, OSAL_STREAM_DEFAULT);
    if (s) goto getout;
    s = osal_stream_write(compressed, data, data_sz, &n_written, OSAL_STREAM_DEFAULT);
    if (s) goto getout;
    if (data_sz != n_written)
    {
        s = OSAL_STATUS_TIMEOUT;
        goto getout;
    }

    /* Write content size and the content.
     */
    data = osal_stream_buffer_content(state.content, &data_sz);
    s = osal_stream_write_long(compressed, data_sz, OSAL_STREAM_DEFAULT);
    if (s) goto getout;
    s = osal_stream_write(compressed, data, data_sz, &n_written, OSAL_STREAM_DEFAULT);
    if (s) goto getout;
    if (data_sz != n_written)
    {
        s = OSAL_STATUS_TIMEOUT;
        goto getout;
    }

getout:
    /* Release temporary buffers.
     */
    osal_stream_buffer_close(state.str);
    osal_stream_buffer_close(state.content);
    osal_stream_buffer_close(state.dictionary);
    return s;
}



static osalStatus osal_parse_json_recursive(
    osalJsonCompressor *state)
{
    os_char c;
    os_long tag_dict_ix;
    osalStatus s;

    do
    {
        if (osal_parse_json_tag(state, &tag_dict_ix)) return OSAL_STATUS_FAILED;
        tag_dict_ix <<= 4;

        /* Skip the colon separating first double quote
         */
        do {
            c = *(state->pos++);
            if (c == '\0') return OSAL_STATUS_FAILED;
        }
        while (osal_char_isspace(c));
        if (c != ':') return OSAL_STATUS_FAILED;

        /* Skip spaces until beginning of value
         */
        do {
            c = *(state->pos++);
            if (c == '\0') return OSAL_STATUS_FAILED;
        }
        while (osal_char_isspace(c));

        /* If this is a block
         */
        if (c == '{')
        {
            s = osal_stream_write_long(state->content, OSAL_JSON_START_BLOCK + tag_dict_ix, 0);
            if (s) return s;
            s = osal_parse_json_recursive(state);
            if (s) return s;
        }

        /* If this is a string
         */
        else if (c == '\"')
        {
            s = osal_parse_json_quoted_string(state);
            if (s) return s;
            s = parse_json_value(state, tag_dict_ix);
            if (s) return s;
        }

        else
        {
            /* Back off first dhavarter of the number, etc.
             */
            state->pos--;

            /* If this is integer or float
             */
            s = osal_parse_json_number(state);
            if (s) return s;
            s = parse_json_value(state, tag_dict_ix);
            if (s) return s;
        }

        /* Skip empty spaces until comma or '}'
         */
        do {
            c = *(state->pos++);
            if (c == '\0') return OSAL_STATUS_FAILED;
        }
        while (osal_char_isspace(c));
        if (c == '}')
        {
            s = osal_stream_write_long(state->content, OSAL_JSON_END_BLOCK, 0);
            return s;
        }
    }
    while (c == ',');

    return OSAL_STATUS_FAILED;
}


static osalStatus osal_parse_json_tag(
    osalJsonCompressor *state,
    os_long *dict_ix)
{
    os_char c;

    /* Skip spaces and the first double quote
     */
    do {
        c = *(state->pos++);
        if (c == '\0') return OSAL_STATUS_FAILED;
    }
    while (osal_char_isspace(c));
    if (c != '\"') return OSAL_STATUS_FAILED;

    osal_parse_json_quoted_string(state);

    *dict_ix = osal_add_string_to_json_dict(state);

    return OSAL_SUCCESS;
}


static osalStatus parse_json_value(
    osalJsonCompressor *state,
    os_long tag_dict_ix)
{
    os_long ivalue, z, value_dict_ix, m, e;
    os_double dvalue;
    os_char *data;
    os_memsz data_n, count;
    osalStatus s;

    data = (os_char*)osal_stream_buffer_content(state->str, &data_n);
    data_n--; /* -1 for terminating '\0'. */

    /* If empty value.
     */
    if (data_n == 0)
    {
        s = osal_stream_write_long(state->content, OSAL_JSON_VALUE_EMPTY + tag_dict_ix, 0);
        return s;
    }

    ivalue = osal_string_to_int(data, &count);
    if (count == data_n)
    {
        if (ivalue == 0)
        {
            z = OSAL_JSON_VALUE_INTEGER_ZERO + tag_dict_ix;
        }
        else if (ivalue == 1)
        {
            z = OSAL_JSON_VALUE_INTEGER_ONE + tag_dict_ix;
        }
        else
        {
            z = OSAL_JSON_VALUE_INTEGER + tag_dict_ix;
        }
        s = osal_stream_write_long(state->content, z, 0);
        if ((z & 15) == OSAL_JSON_VALUE_INTEGER && !s)
        {
            s = osal_stream_write_long(state->content, ivalue, 0);
        }

        return s;
    }

    dvalue = osal_string_to_double(data, &count);
    if (count == data_n)
    {
        if (dvalue == 0.0)
        {
            z = OSAL_JSON_VALUE_INTEGER_ZERO + tag_dict_ix;
        }
        else if (dvalue == 1.0)
        {
            z = OSAL_JSON_VALUE_INTEGER_ONE + tag_dict_ix;
        }
        else
        {
            z = OSAL_JSON_VALUE_FLOAT + tag_dict_ix;
        }
        s = osal_stream_write_long(state->content, z, 0);
        if (s) return s;
        if ((z & 15) == OSAL_JSON_VALUE_FLOAT)
        {
            osal_double2ints(dvalue, &m, &e);
            s = osal_stream_write_long(state->content, m, 0);
            if (m && !s)
            {
                s = osal_stream_write_long(state->content, e, 0);
            }
        }

        return s;
    }

    s = osal_stream_write_long(state->content, OSAL_JSON_VALUE_STRING + tag_dict_ix, 0);
   if (s) return s;
    value_dict_ix = osal_add_string_to_json_dict(state);
    s = osal_stream_write_long(state->content, value_dict_ix, 0);
    return s;
}


static osalStatus osal_parse_json_quoted_string(
    osalJsonCompressor *state)
{
    os_long seekzero = 0;
    os_memsz n_written;
    osalStatus s;
    os_char c;

    osal_stream_buffer_seek(state->str, &seekzero,
        OSAL_STREAM_SEEK_WRITE_POS|OSAL_STREAM_SEEK_SET);

    do
    {
        c = *(state->pos++);

        /* If c is quote character
         */
        if (c == '\\')
        {
            c = *(state->pos++);
        }

        /* If end of the string
         */
        else if (c == '\"')
        {
            s = osal_stream_buffer_write(state->str, (os_uchar*)"\0", 1, &n_written, 0);
            return s;
        }

        /* If unexpected end of file
         */
        else if (c == '\0')
        {
            return OSAL_STATUS_FAILED;
        }

        /* Store the character. Make sure that we have room
         * in buffer for this character and terminating '\0'.
         */
        s = osal_stream_buffer_write(state->str, (os_uchar*)&c, 1, &n_written, 0);
    }
    while (!s);

    return s;
}

static osalStatus osal_parse_json_number(
    osalJsonCompressor *state)
{
    os_char *p, c;
    os_long seekzero = 0;
    os_memsz n_written;
    osalStatus s;

    osal_stream_buffer_seek(state->str, &seekzero,
        OSAL_STREAM_SEEK_WRITE_POS|OSAL_STREAM_SEEK_SET);

    /* Skip the colon separating first double quote
     */
    p = state->pos;
    while (OS_TRUE)
    {
        c = *p;
        if (c == '\0') return OSAL_STATUS_FAILED;
        if (osal_char_isspace(c) || c == ',' || c == '}') break;
        p++;
    }

    s = osal_stream_buffer_write(state->str, (os_uchar*)state->pos, p - state->pos, &n_written, 0);
    if (!s) s = osal_stream_buffer_write(state->str, (os_uchar*)"\0", 1, &n_written, 0);

    state->pos = p;
    return s;
}

/* Very inefficient loop trough whole list: switch to B-tree or hash tab
 */
static os_long osal_add_string_to_json_dict(
    osalJsonCompressor *state)
{
    os_char *data, *newstr;
    os_memsz data_sz, newstr_sz, n_written;
    os_int pos, ix, bytes;
    os_long len;
    osalStatus s;

    newstr = (os_char*)osal_stream_buffer_content(state->str, &newstr_sz);
    newstr_sz--;
    data = (os_char*)osal_stream_buffer_content(state->dictionary, &data_sz);

    /* Try to locate
     */
    pos = 0;
    ix = 0;
    while (pos < data_sz)
    {
        bytes = osal_intser_reader(data + pos, &len);
        if (len == newstr_sz)
        {
            if (!os_memcmp(newstr, data + pos + bytes, newstr_sz))
            {
                return ix;
            }
        }
        pos += bytes + len;
        ix++;
    }

    s = osal_stream_write_long(state->dictionary, newstr_sz, OSAL_STREAM_DEFAULT);
    if (!s) s = osal_stream_buffer_write(state->dictionary, (os_uchar*)newstr, newstr_sz, &n_written, 0);
    return s;
}


#endif
