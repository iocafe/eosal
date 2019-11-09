/**

  @file    osal_json_compress_test.c
  @brief   Test JSON compression to packed binary.
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

/* Forward referred static functions.
 */
static osalStatus osal_json_from_text_to_binary(
    os_char *src_path,
    os_char *dst_path,
    os_char *skip_tags,
    os_int flags);

static osalStatus osal_json_from_binary_to_text(
    os_char *src_path,
    os_char *dst_path);

static void osal_json_util_help(void);


/**
****************************************************************************************************

  @brief Process entry point.

  The osal_int64_test() function is OS independent entry point.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
osalStatus osal_main(
    os_int argc,
    os_char *argv[])
{
    os_char *src_path, *dst_path, *extras;
    os_memsz extras_sz, n_written;
    osalStatus s;
    osalStream extra_args = OS_NULL;
    os_int i, path_nr, flags;

    enum
    {
        JSON_T2B,
        JSON_B2T
    }
    op;

    op = JSON_T2B;
    src_path = ".stdin";
    dst_path = ".stdout";
    path_nr = 0;
    flags = 0;

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (!os_strnicmp(argv[i], "--t2b", -1))
            {
                op = JSON_T2B;
            }
            else if (!os_strnicmp(argv[i], "--b2t", -1))
            {
                op = JSON_B2T;
            }
            else if (!os_strnicmp(argv[i], "--keep-quirks", -1))
            {
                flags |= OSAL_JSON_KEEP_QUIRKS;
            }
            else if (!os_strcmp(argv[i], "-?") ||
                !os_strnicmp(argv[i], "-h", -1) ||
                !os_strnicmp(argv[i], "--help", -1))
            {
                osal_json_util_help();
                s = OSAL_SUCCESS;
                goto getout;
            }
            else
            {
                if (extra_args == OS_NULL)
                {
                    extra_args = osal_stream_buffer_open(OS_NULL,
                        OS_NULL, OS_NULL, OSAL_STREAM_DEFAULT);
                }
                else
                {
                    osal_stream_print_str(extra_args, ",", 0);
                }
                osal_stream_print_str(extra_args, argv[i]+1, 0);
                osal_stream_write(extra_args, "\0", 1, &n_written, 0);
            }
        }
        else
        {
            switch (path_nr++)
            {
                case 0: src_path = argv[i]; break;
                case 1: dst_path = argv[i]; break;
            }
        }
    }

    switch (op)
    {
        default:
        case JSON_T2B:
            extras = OS_NULL;
            if (extra_args)
            {
                extras = osal_stream_buffer_content(extra_args, &extras_sz);
            }
            s = osal_json_from_text_to_binary(src_path, dst_path, extras, flags);
            break;

        case JSON_B2T:
            s = osal_json_from_binary_to_text(src_path, dst_path);
            break;
    }

    if (path_nr > 1 || s)
        osal_console_write(s ? "\nFAILED\n" : "\nsuccess\n");

getout:
    if (extra_args)
    {
        osal_stream_close(extra_args);
    }

    return s;
}


/**
****************************************************************************************************

  @brief Conver JSON text file to packed binary JSON file.

  The osal_json_from_text_to_binary() function..

  @param   src_path Path to input file.
  @param   dst_path Path to output file.
  @param   skip_tags List of JSON tags to ignore (not to include in packed binary),
           for example "title,help".

  @return  OSAL_SUCCESS (0) is all is fine. Other values indicate an error.

****************************************************************************************************
*/
static osalStatus osal_json_from_text_to_binary(
    os_char *src_path,
    os_char *dst_path,
    os_char *skip_tags,
    os_int flags)
{
    os_char *json_text = OS_NULL;
    os_memsz json_text_sz = 0;
    osalStream compressed = OS_NULL;
    osalStatus s = OSAL_STATUS_FAILED;

    json_text = (os_char*)os_read_file_alloc(src_path, &json_text_sz, OS_FILE_NULL_CHAR);
    if (json_text == OS_NULL) goto getout;

    compressed = osal_file_open(dst_path, OS_NULL, OS_NULL, OSAL_STREAM_WRITE);
    if (compressed == OS_NULL) goto getout;

    s = osal_compress_json(compressed, json_text, skip_tags, flags);

getout:
    os_free(json_text, json_text_sz);
    osal_stream_close(compressed);

    return s;
}


/**
****************************************************************************************************

  @brief Conver packed binary JSON file to plain JSON text.

  The osal_json_from_binary_to_text() function..

  @param   src_path Path to input file.
  @param   dst_path Path to output file.

  @return  OSAL_SUCCESS (0) is all is fine. Other values indicate an error.

****************************************************************************************************
*/
static osalStatus osal_json_from_binary_to_text(
    os_char *src_path,
    os_char *dst_path)
{
    os_char *json_binary = OS_NULL;
    os_memsz json_binary_sz = 0;
    osalStream uncompressed = OS_NULL;
    osalStatus s = OSAL_STATUS_FAILED;

    json_binary = (os_char*)os_read_file_alloc(src_path, &json_binary_sz, OS_FILE_DEFAULT);
    if (json_binary == OS_NULL) goto getout;

    uncompressed = osal_file_open(dst_path, OS_NULL, OS_NULL, OSAL_STREAM_WRITE);
    if (uncompressed == OS_NULL) goto getout;

    s = osal_uncompress_json(uncompressed, json_binary, json_binary_sz, 0);

getout:
    os_free(json_binary, json_binary_sz);
    osal_stream_close(uncompressed);

    return s;
}


/**
****************************************************************************************************

  @brief Show brief command line help.

  The osal_json_util_help() function.
  @return  None.

****************************************************************************************************
*/
static void osal_json_util_help(void)
{
    static os_char text[] = {
        "json [--t2b] [--b2t] [-title] [infile] [outfile]\n"
        "Convert: JSON file/binary file/C source file\n"
        "--t2b JSON fom text file to packed binary format (default)\n"
        "--b2t Packed binary JSON to plain text JSON\n"
        "--keep-quirks With --t2b skips keeps marking like null, false, true (not changed to "", 0, 1)\n"
        "-title With --t2b skips \"title\" tags (other tags can be skipped the same way)\n"};

    osal_console_write(text);
}
