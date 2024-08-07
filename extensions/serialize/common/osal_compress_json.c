/**

  @file    osal_compress_json.c
  @brief   Compress JSON as binary data.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
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
    /** Current position in JSON source.
     */
    const os_char *pos;

    /** Buffer for string being parsed
     */
    osalStream str;

    /** Buffer for content being created.
     */
    osalStream content;

    /** Buffer for dictionary strings.
     */
    osalStream dictionary;

    /** Start string positions for dictionary.
     */
    osalStream dict_pos;

    /** Number of strings in dictionary.
     */
    os_int dictionary_n;

    /** List of tags to skip.
     */
    const os_char *skip_tags;

    /** How deep we have recursed into skipped tags.
     */
    os_int skip_count;

    /** OSAL_JSON_KEEP_QUIRKS not keep compressed JSON
        as much as original without simplifying it.
     */
    os_int flags;

    /* Current tag is "password
     */
    os_boolean is_password;
}
osalJsonCompressor;


/* Forward referred static functions.
 */
static osalStatus osal_parse_json_recursive(
    osalJsonCompressor *state,
    os_boolean expect_array);

static osalStatus osal_parse_json_tag(
    osalJsonCompressor *state,
    os_long *dict_ix);

static osalStatus parse_json_value(
    osalJsonCompressor *state,
    os_long tag_dict_ix,
    os_boolean in_quotes);

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
  @param  flags OSAL_JSON_KEEP_QUIRKS not keep compressed JSON as much as original without
          simplifying empty values, etc. OSAL_JSON_HASH_PASSWORDS turns on hasing passwords.
           Set 0 for default operation.
  @return OSAL_SUCCESS to indicate success. Other return values indicate an error.

****************************************************************************************************
*/
osalStatus osal_compress_json(
    osalStream compressed,
    const os_char *json_source,
    const os_char *skip_tags,
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

#if OSAL_SECRET_SUPPORT == 0
    osal_debug_assert((flags & OSAL_JSON_HASH_PASSWORDS) == 0)
#endif

    os_memclear(&state, sizeof(state));
    state.pos = json_source;
    state.skip_tags = skip_tags;
    state.flags = flags;

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
    s = osal_parse_json_recursive(&state, OS_FALSE);
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
    osal_stream_buffer_close(state.str, OSAL_STREAM_DEFAULT);
    osal_stream_buffer_close(state.content, OSAL_STREAM_DEFAULT);
    osal_stream_buffer_close(state.dictionary, OSAL_STREAM_DEFAULT);
    osal_stream_buffer_close(state.dict_pos, OSAL_STREAM_DEFAULT);
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
  @param  expect_array OS_TRUE if we are parsing array within []
  @return OSAL_SUCCESS to indicate success. Other return values indicate an error.

****************************************************************************************************
*/
static osalStatus osal_parse_json_recursive(
    osalJsonCompressor *state,
    os_boolean expect_array)
{
    os_char c;
    os_long tag_dict_ix;
    osalStatus s, tag_s;

    do
    {
        state->is_password = OS_FALSE;
        if (!expect_array)
        {
            /* Parse tag. If we need to ignore tag content.
             */
            tag_s = osal_parse_json_tag(state, &tag_dict_ix);
            if (tag_s == OSAL_NOTHING_TO_DO)
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
        }
        else
        {
            tag_s = OSAL_SUCCESS;
            tag_dict_ix = OSAL_JSON_DICT_NONE;
        }

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

            s = osal_parse_json_recursive(state, OS_FALSE);
            if (s) return s;

            if (!state->skip_count)
            {
                s = osal_stream_write_long(state->content, OSAL_JSON_END_BLOCK, 0);
                if (s) return s;
            }
        }

        /* If this is an array
         */
        else if (c == '[')
        {
            if (!state->skip_count)
            {
                s = osal_stream_write_long(state->content, OSAL_JSON_START_ARRAY + tag_dict_ix, 0);
                if (s) return s;
            }

            s = osal_parse_json_recursive(state, OS_TRUE);
            if (s) return s;

            if (!state->skip_count)
            {
                s = osal_stream_write_long(state->content, OSAL_JSON_END_ARRAY, 0);
                if (s) return s;
            }
        }

        /* Empty array or dictionary.
         */
        else if (c == ']' || c == '}')
        {
            return OSAL_SUCCESS;
        }

        /* If this is a string
         */
        else if (c == '\"')
        {
            s = osal_parse_json_quoted_string(state);
            if (s) return s;
            s = parse_json_value(state, tag_dict_ix, OS_TRUE);
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
            s = parse_json_value(state, tag_dict_ix, OS_FALSE);
            if (s) return s;
        }

        /* End ignoring tag content.
         */
        if (tag_s == OSAL_NOTHING_TO_DO)
        {
            state->skip_count--;
        }

        /* Skip empty spaces until comma, '}' or ']'
         */
        do {
            c = *(state->pos++);
            if (c == '\0') return OSAL_STATUS_FAILED;
        }
        while (osal_char_isspace(c));
        if (c == '}' || c == ']')
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
            return OSAL_NOTHING_TO_DO;
        }

#if OSAL_SECRET_SUPPORT
        if (state->flags & OSAL_JSON_HASH_PASSWORDS)
        {
            if (!os_strcmp(data, "password"))
            {
                state->is_password = OS_TRUE;
            }
        }
#endif
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
  @param  in_quotes If not in quotes, allow null word to mean empty value flag and
          convert numbers.
  @return OSAL_SUCCESS to indicate success. Other return values indicate an error.

****************************************************************************************************
*/
static osalStatus parse_json_value(
    osalJsonCompressor *state,
    os_long tag_dict_ix,
    os_boolean in_quotes)
{
    os_long ivalue, z, value_dict_ix, m;
    os_short e;
    os_double dvalue;
    os_char *data;
    os_memsz data_n, count;
    os_int flags;
    osalStatus s;
#if OSAL_SECRET_SUPPORT
    os_char hashbuf[OSAL_SECRET_STR_SZ];
    os_long seekzero = 0;
    os_memsz n_written;
#endif

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

#if OSAL_SECRET_SUPPORT
    /* If this is password and not "auto", and we are hashing passwords.
     */
    if (state->is_password)
    {
        if (os_strcmp(data, "auto") && os_strcmp(data, osal_str_asterisk) && os_strcmp(data, osal_str_empty))
        {
            osal_hash_password(hashbuf, data, sizeof(hashbuf));
            osal_stream_buffer_seek(state->str, &seekzero, OSAL_STREAM_SEEK_WRITE_POS|OSAL_STREAM_SEEK_SET);
            osal_stream_buffer_write(state->str, hashbuf, os_strlen(hashbuf), &n_written, 0);
            data = osal_stream_buffer_content(state->str, &data_n);
            data_n--; /* -1 for terminating '\0'. */
        }
    }
#endif

    if (!in_quotes)
    {
        ivalue = osal_str_to_int(data, &count);
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

        dvalue = osal_str_to_double(data, &count);
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

        flags = state->flags;
        if (!os_strcmp(data, "null"))
        {
            s = osal_stream_write_long(state->content,
                ((flags & OSAL_JSON_KEEP_QUIRKS) ? OSAL_JSON_VALUE_NULL : OSAL_JSON_VALUE_EMPTY) + tag_dict_ix, 0);
            return s;
        }
        else if (!os_strcmp(data, "true"))
        {
            s = osal_stream_write_long(state->content,
                ((flags & OSAL_JSON_KEEP_QUIRKS) ? OSAL_JSON_VALUE_TRUE : OSAL_JSON_VALUE_INTEGER_ONE) + tag_dict_ix, 0);
            return s;
        }
        else if (!os_strcmp(data, "false"))
        {
            s = osal_stream_write_long(state->content,
                ((flags & OSAL_JSON_KEEP_QUIRKS) ? OSAL_JSON_VALUE_FALSE: OSAL_JSON_VALUE_INTEGER_ZERO) + tag_dict_ix, 0);
            return s;
        }
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
#if OSAL_UTF8
    os_uint c32;
    os_char ubuf[5], substr[6];
    os_memsz count, n;
#endif

    osal_stream_buffer_seek(state->str, &seekzero,
        OSAL_STREAM_SEEK_WRITE_POS|OSAL_STREAM_SEEK_SET);

    do
    {
        c = *(state->pos++);

        /* If c is escape character?
         */
        if (c == '\\')
        {
            c = *(state->pos++);
            switch (c)
            {
                case '\0': return OSAL_STATUS_FAILED;
                default: break;
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case 't': c = '\t'; break;
#if OSAL_UTF8
                case 'u':
                    os_memcpy(ubuf, state->pos, 4);
                    ubuf[4] = '\0';
                    c32 = (os_uint)osal_hex_str_to_int(ubuf, &count);
                    if (count != 4) return OSAL_STATUS_FAILED;
                    n = osal_char_utf32_to_utf8(substr, sizeof(substr), c32);
                    s = osal_stream_buffer_write(state->str, substr, n, &n_written, 0);
                    if (s) return s;
                    state->pos += 4;
                    goto goon;
#endif
            }
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
goon:;
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
    os_char c;
    const os_char *p;
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
