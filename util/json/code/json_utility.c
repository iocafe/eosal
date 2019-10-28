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

static osalStatus osal_json_from_text_to_binary(
    os_char *src_path,
    os_char *dst_path,
    os_char *skip_tags);


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
    osalStatus s;

    s = osal_json_from_text_to_binary("/tmp/nuke.json", "/tmp/nuke.json-bin", "title");
    if (!s)
        osal_console_write("SUKSET");


    return OSAL_SUCCESS;
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
    os_char *skip_tags)
{
    os_char *json_text = OS_NULL;
    os_memsz json_text_sz = 0;
    osalStream compressed = OS_NULL;
    osalStatus s = OSAL_STATUS_FAILED;

    json_text = (os_char*)os_read_file_alloc(src_path, &json_text_sz, OS_FILE_NULL_CHAR);
    if (json_text == OS_NULL) goto getout;

    compressed = osal_file_open(dst_path, OS_NULL, OS_NULL, OSAL_STREAM_WRITE);
    if (compressed == OS_NULL) goto getout;

    s = osal_compress_json(compressed, json_text, skip_tags, 0);

getout:
    os_free(json_text, json_text_sz);
    osal_stream_close(compressed);

    return s;
}

/*
    uncompressed = osal_file_open(dst_path, OS_NULL, OS_NULL, OSAL_STREAM_WRITE);
    if (uncompressed == OS_NULL) goto getout;

    s = osal_uncompress_json(uncompressed, data, data_sz, 0);
    if (s) goto getout;
*/
