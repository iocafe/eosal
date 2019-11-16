/**

  @file    osal_json_compress_test.c
  @brief   Test JSON compression to packed binary.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    25.10.2019

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "osal_example_collection_main.h"


const char example_json_path[] = "/coderoot/eosal/examples/ecollection/test_files/example.json";


/**
****************************************************************************************************

  @brief Process entry point.

  The osal_int64_test() function is OS independent entry point.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
osalStatus osal_json_compress_test(
    os_int argc,
    os_char *argv[])
{
    osalStatus s;
    osalStream compressed = OS_NULL, uncompressed = OS_NULL;
    os_char nbuf[OSAL_NBUF_SZ];
    os_char *data, *str, *json_text;
    os_memsz data_sz, str_sz, json_text_sz;

    json_text = os_read_file_alloc(example_json_path, &json_text_sz, OS_FILE_DEFAULT);
    if (json_text == OS_NULL)
    {
        osal_console_write("reading file failed: ");
        osal_console_write(example_json_path);
        return OSAL_STATUS_FAILED;
    }

    compressed = osal_stream_buffer_open(OS_NULL, OS_NULL, OS_NULL, OSAL_STREAM_DEFAULT);

    s = osal_compress_json(compressed, json_text, "title", OSAL_JSON_KEEP_QUIRKS /* OSAL_JSON_SIMPLIFY */);
    if (s)
    {
        osal_console_write("osal_compress_json() failed\n");
        goto getout;
    }

    osal_int_to_string(nbuf, sizeof(nbuf), s);
    osal_console_write("\nstatus = ");
    osal_console_write(nbuf);

    osal_console_write("\noriginal size = ");
    osal_int_to_string(nbuf, sizeof(nbuf), os_strlen(json_text));
    osal_console_write(nbuf);

    osal_console_write("\ncompressed size = ");
    data = osal_stream_buffer_content(compressed, &data_sz);
    osal_int_to_string(nbuf, sizeof(nbuf), data_sz);
    osal_console_write(nbuf);

    osal_console_write("\n");

    uncompressed = osal_stream_buffer_open(OS_NULL, OS_NULL, OS_NULL, OSAL_STREAM_DEFAULT);

    s = osal_uncompress_json(uncompressed, data, data_sz, 0);
    if (s)
    {
        osal_console_write("osal_uncompress_json() failed\n");
    }
    else
    {
        str = osal_stream_buffer_content(uncompressed, &str_sz);
        osal_console_write(str);
    }

getout:
    osal_stream_buffer_close(uncompressed);
    osal_stream_buffer_close(compressed);

    os_free(json_text, json_text_sz);
    return s;
}
