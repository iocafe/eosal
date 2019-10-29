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
#if OSAL_JSON_TEXT_SUPPORT


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

    /* Buffer for dictionary strings.
     */
    osalStream dictionary;

    /* Start string positions for dictionary.
     */
    osalStream dict_pos;

    /* Number of strings in dictionary.
     */
    os_int dictionary_n;

    os_char *skip_tags;

    os_int skip_count;
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
    os_long tag_dict_ix,
    os_boolean allow_null);

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
  @param  json_source JSON content as plain text.
  @param  skip_tags List of tags to skip (not include in compressed data). For example
          "title,help" to exclude title and help texts from compressed binary JSON.
  @param  flags Reserved for future, set 0 for now.
  @return OSAL_SUCCESS to indicate success. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus osal_compress_json(
    osalStream compressed,
    os_char *json_source,
    os_char *skip_tags,
    os_int flags)
{
    osalJsonCompressor state;
    os_char c;
    os_char *data;
    os_memsz data_sz, n_written;
    osalStatus s = OSAL_STATUS_FAILED;
    os_ushort checksum;
    os_char tmp[OSAL_INTSER_BUF_SZ];
    os_int tmp_n;

    os_memclear(&state, sizeof(state));
    state.pos = json_source;
    state.skip_tags = skip_tags;

    /* Allocate temporary buffers for parsing string, generating content and for dictionary.
     */
    state.str = osal_stream_buffer_open(OS_NULL, OS_NULL, OS_NULL, OSAL_STREAM_DEFAULT);
    state.content = osal_stream_buffer_open(OS_NULL, OS_NULL, OS_NULL, OSAL_STREAM_DEFAULT);
    state.dictionary = osal_stream_buffer_open(OS_NULL, OS_NULL, OS_NULL, OSAL_STREAM_DEFAULT);
    state.dict_pos = osal_stream_buffer_open(OS_NULL, OS_NULL, OS_NULL, OSAL_STREAM_DEFAULT);
    if (state.str == OS_NULL || state.content == OS_NULL || state.dictionary == OS_NULL
        || state.dict_pos == OS_NULL)
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
    checksum = OSAL_CHECKSUM_INIT;
    data = osal_stream_buffer_content(state.dictionary, &data_sz);
    tmp_n = osal_intser_writer(tmp, data_sz);
    s = osal_stream_write(compressed, tmp, tmp_n, &n_written, OSAL_STREAM_DEFAULT);
    if (s) goto getout;
    if (tmp_n != n_written) goto timeout;
    os_checksum(tmp, tmp_n, &checksum);
    s = osal_stream_write(compressed, data, data_sz, &n_written, OSAL_STREAM_DEFAULT);
    if (s) goto getout;
    if (data_sz != n_written) goto timeout;
    os_checksum(data, data_sz, &checksum);

    /* Write content size and the content.
     */
    data = osal_stream_buffer_content(state.content, &data_sz);
    tmp_n = osal_intser_writer(tmp, data_sz);
    s = osal_stream_write(compressed, tmp, tmp_n, &n_written, OSAL_STREAM_DEFAULT);
    if (s) goto getout;
    if (tmp_n != n_written) goto timeout;
    os_checksum(tmp, tmp_n, &checksum);
    s = osal_stream_write(compressed, data, data_sz, &n_written, OSAL_STREAM_DEFAULT);
    if (s) goto getout;
    if (data_sz != n_written) goto timeout;
    os_checksum(data, data_sz, &checksum);

    /* Write checksum.
     */
    s = osal_stream_write(compressed, (os_char*)&checksum, sizeof(os_ushort),
        &n_written, OSAL_STREAM_DEFAULT);
    if (s) goto getout;
    if (sizeof(os_ushort) != n_written) goto timeout;

getout:
    /* Release temporary buffers.
     */
    osal_stream_buffer_close(state.str);
    osal_stream_buffer_close(state.content);
    osal_stream_buffer_close(state.dictionary);
    osal_stream_buffer_close(state.dict_pos);
    return s;

timeout:
    s = OSAL_STATUS_TIMEOUT;
    goto getout;
}


/**
****************************************************************************************************

  @brief Parse a JSON block.
  @anchor osal_parse_json_recursive

  The osal_parse_json_recursive() function recursively parses a block within {} from plain
  JSON text into compression state.

  @param  state JSON compression state.
  @return OSAL_SUCCESS to indicate success. Other return values indicate an error.

****************************************************************************************************
*/
static osalStatus osal_parse_json_recursive(
    osalJsonCompressor *state)
{
    os_char c;
    os_long tag_dict_ix;
    osalStatus s, tag_s;

    do
    {
        /* Parse tag. If we need to ignore tag content.
         */
        tag_s = osal_parse_json_tag(state, &tag_dict_ix);
        if (tag_s == OSAL_STATUS_NOTHING_TO_DO)
        {
            state->skip_count++;
        }
        else
        {
            if (tag_s) return tag_s;
        }

        tag_dict_ix <<= OSAL_JSON_CODE_SHIFT;

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
            if (!state->skip_count)
            {
                s = osal_stream_write_long(state->content, OSAL_JSON_START_BLOCK + tag_dict_ix, 0);
                if (s) return s;
            }

            s = osal_parse_json_recursive(state);
            if (s) return s;

            if (!state->skip_count)
            {
                s = osal_stream_write_long(state->content, OSAL_JSON_END_BLOCK, 0);
                if (s) return s;
            }
        }

        /* If this is a string
         */
        else if (c == '\"')
        {
            s = osal_parse_json_quoted_string(state);
            if (s) return s;
            s = parse_json_value(state, tag_dict_ix, OS_FALSE);
            if (s) return s;
        }

        else
        {
            /* Back off first chararter of the number, etc.
             */
            state->pos--;

            /* If this is integer or float
             */
            s = osal_parse_json_number(state);
            if (s) return s;
            s = parse_json_value(state, tag_dict_ix, OS_TRUE);
            if (s) return s;
        }

        /* End ignoring tag content.
         */
        if (tag_s == OSAL_STATUS_NOTHING_TO_DO)
        {
            state->skip_count--;
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
            return OSAL_SUCCESS;
        }
    }
    while (c == ',');

    return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Parse a JSON tag name.
  @anchor osal_parse_json_tag

  The osal_parse_json_tag() function parses a quoted tag name from JSON text.

  @param  state JSON compression state.
  @param  dict_ix Pointer to integer where to store tag position in dictionary. This can be
          static dictionary item (values 0 ... OSAL_JSON_DICT_N_STATIC - 1), or byte index
          within dictionary + OSAL_JSON_DICT_N_STATIC.
  @return OSAL_SUCCESS to indicate success. Other return values indicate an error.

****************************************************************************************************
*/
static osalStatus osal_parse_json_tag(
    osalJsonCompressor *state,
    os_long *dict_ix)
{
    os_char c, *data;
    os_memsz data_n;

    /* Skip spaces and the first double quote
     */
    do {
        c = *(state->pos++);
        if (c == '\0') return OSAL_STATUS_FAILED;
    }
    while (osal_char_isspace(c));
    if (c != '\"') return OSAL_STATUS_FAILED;

    osal_parse_json_quoted_string(state);

    /* If this tag is on the skip list
     */
    if (state->skip_tags)
    {
        data = osal_stream_buffer_content(state->str, &data_n);
        if (os_strstr(state->skip_tags, data, OSAL_STRING_SEARCH_ITEM_NAME))
        {
            *dict_ix = 0;
            return OSAL_STATUS_NOTHING_TO_DO;
        }
    }

    *dict_ix = osal_add_string_to_json_dict(state);

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Parse a JSON value.
  @anchor parse_json_value

  The parse_json_value() function parses a value from JSON. The value may be quoted or unquoted
  string, integer, float, or null to indicate empty value.

  @param  state JSON compression state.
  @param  tag_dict_ix Tag position in dictionary. This can be static dictionary item (values 0 ...
          OSAL_JSON_DICT_N_STATIC - 1), or byte index within dictionary + OSAL_JSON_DICT_N_STATIC.
  @param  allow_null Allow null word to mean empty value flag. This flag is true when parsing
          unquoted value and false when parsing a value within quotes.
  @return OSAL_SUCCESS to indicate success. Other return values indicate an error.

****************************************************************************************************
*/
static osalStatus parse_json_value(
    osalJsonCompressor *state,
    os_long tag_dict_ix,
    os_boolean allow_null)
{
    os_long ivalue, z, value_dict_ix, m;
    os_short e;
    os_double dvalue;
    os_char *data;
    os_memsz data_n, count;
    osalStatus s;

    /* If we are ignoring the tag content.
     */
    if (state->skip_count) return OSAL_SUCCESS;

    data = osal_stream_buffer_content(state->str, &data_n);
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
        if ((z & OSAL_JSON_CODE_MASK) == OSAL_JSON_VALUE_INTEGER && !s)
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
        if ((z & OSAL_JSON_CODE_MASK) == OSAL_JSON_VALUE_FLOAT)
        {
            osal_float2ints((os_float)dvalue, &m, &e);
            s = osal_stream_write_long(state->content, m, 0);
            if (m && !s)
            {
                s = osal_stream_write_long(state->content, e, 0);
            }
        }

        return s;
    }

    if (allow_null) if (!os_strcmp(data, "null"))
    {
        s = osal_stream_write_long(state->content, OSAL_JSON_VALUE_EMPTY + tag_dict_ix, 0);
        return s;
    }

    s = osal_stream_write_long(state->content, OSAL_JSON_VALUE_STRING + tag_dict_ix, 0);
    if (s) return s;
    value_dict_ix = osal_add_string_to_json_dict(state);
    s = osal_stream_write_long(state->content, value_dict_ix, 0);
    return s;
}


/**
****************************************************************************************************

  @brief Parse a JSON string in quotes.
  @anchor osal_parse_json_quoted_string

  The osal_parse_json_quoted_string() function parses a quoted string value from JSON.

  @param  state JSON compression state.
  @return OSAL_SUCCESS to indicate success. Other return values indicate an error.

****************************************************************************************************
*/
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
            s = osal_stream_buffer_write(state->str, "\0", 1, &n_written, 0);
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
        s = osal_stream_buffer_write(state->str, &c, 1, &n_written, 0);
    }
    while (!s);

    return s;
}


/**
****************************************************************************************************

  @brief Parse a number.
  @anchor osal_parse_json_number

  The osal_parse_json_number() function parses a number (or null) string as value.

  @param  state JSON compression state.
  @return OSAL_SUCCESS to indicate success. Other return values indicate an error.

****************************************************************************************************
*/
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

    s = osal_stream_buffer_write(state->str, state->pos, p - state->pos, &n_written, 0);
    if (!s) s = osal_stream_buffer_write(state->str, "\0", 1, &n_written, 0);

    state->pos = p;
    return s;
}


/**
****************************************************************************************************

  @brief Make sure that a string is in dictionary.
  @anchor osal_add_string_to_json_dict

  The osal_add_string_to_json_dict() function checks first if this is static dictionary item,
  if so returns the static dictionary index.
  Then the dictonary for this document is checked. If found there, the function returns
  position in dictionary as offset from beginning of dictionary plus OSAL_JSON_DICT_N_STATIC
  offset.

  Note: Very inefficient loop trough whole list if dictionary is large: switch to B-tree or
  to hash table.

  @param  state JSON compression state.
  @return String position in dictionary. This can be static dictionary item (values 0 ...
          OSAL_JSON_DICT_N_STATIC - 1), or byte index within dictionary + OSAL_JSON_DICT_N_STATIC.

****************************************************************************************************
*/
static os_long osal_add_string_to_json_dict(
    osalJsonCompressor *state)
{
    os_char *newstr, *dictionary, *dict_pos;
    os_memsz dictionary_sz, dict_pos_sz, newstr_sz, n_written;
    os_int pos, ix;
    os_long endpos;
    osalStatus s;

    /* If we are ignoring the tag content.
     */
    if (state->skip_count) return 0;

    newstr = osal_stream_buffer_content(state->str, &newstr_sz);

    ix = osal_find_in_static_json_dict(newstr);
    if (ix != OSAL_JSON_DICT_NO_ENTRY) return ix;

    dictionary = osal_stream_buffer_content(state->dictionary, &dictionary_sz);
    dict_pos = osal_stream_buffer_content(state->dict_pos, &dict_pos_sz);

    /* Try to locate
     */
    for (ix = 0; ix < state->dictionary_n; ix++)
    {
        os_memcpy(&pos, dict_pos + ix * sizeof(os_int), sizeof(os_int));
        if (!os_strcmp(newstr, dictionary + pos))
        {
            return (os_long)pos + OSAL_JSON_DICT_N_STATIC;
        }
    }

    osal_stream_buffer_seek(state->dictionary, &endpos, OSAL_STREAM_SEEK_WRITE_POS);
    pos = (os_int)endpos;
    s = osal_stream_write(state->dict_pos, (os_char*)&pos, sizeof(os_int), &n_written, OSAL_STREAM_DEFAULT);
    if (!s) s = osal_stream_buffer_write(state->dictionary, newstr, newstr_sz, &n_written, 0);
    state->dictionary_n++;
    return endpos + OSAL_JSON_DICT_N_STATIC;
}

#endif
