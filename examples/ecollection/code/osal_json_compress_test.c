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
#include "osal_json_compress_test.h"

static os_char json_text[] = {
"{\n"
"  \"pins\": {\n"
"    \"name\": \"jane\",\n"
"    \"title\": \"IO pin setup for 'jane' application on 'carol' hardware\",\n"
"    \"inputs\": {\n"
"	  \"DIP_SWITCH_3\": {\"addr\": 34, \"pull-up\": 1},\n"
"	  \"DIP_SWITCH_4\": {\"addr\": 35},\n"
"	  \"TOUCH_SENSOR\": {\"addr\": 4, \"touch\": 1}\n"
"    },\n"
"    \"outputs\": {\n"
"	  \"LED_BUILTIN\": {\"addr\": 2}\n"
"    },\n"
"    \"analog_inputs\": {\n"
"	  \"POTENTIOMETER\": {\"addr\": 25, \"speed\": 3, \"delay\": 11, \"max\": 4095}\n"
"    },\n"
"    \"pwm\": {\n"
"	  \"SERVO\": {\"bank\": 0, \"addr\": 32, \"frequency\": 50, \"resolution\": 12, \"init\": 2048, \"max\": 4095},\n"
"	  \"DIMMER_LED\": {\"bank\": 1, \"addr\": 33, \"frequency\": 5000, \"resolution\": 12, \"init\": 0, \"max\": 4095}\n"
"    }\n"
"  }\n"
"}\n"};


/**
****************************************************************************************************

  @brief Process entry point.

  The osal_int64_test() function is OS independent entry point.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
os_int osal_json_compress_test(
    os_int argc,
    os_char *argv[])
{
    osalStatus s;
    osalStream compressed;
    os_char nbuf[OSAL_NBUF_SZ];
    os_char *data;
    os_memsz data_sz;


    osal_console_write(json_text);


    compressed = osal_stream_buffer_open(OS_NULL, OS_NULL, OS_NULL, OSAL_STREAM_DEFAULT);

    s = osal_compress_json(compressed, json_text);
    osal_int_to_string(nbuf, sizeof(nbuf), s);
    osal_console_write("\nstatus = ");
    osal_console_write(nbuf);

    osal_console_write("\nogiginal size = ");
    osal_int_to_string(nbuf, sizeof(nbuf), os_strlen(json_text));
    osal_console_write(nbuf);

    osal_console_write("\ncompressed size = ");
    data = (os_char*)osal_stream_buffer_content(compressed, &data_sz);
    osal_int_to_string(nbuf, sizeof(nbuf), data_sz);
    osal_console_write(nbuf);

    osal_console_write("\n");

    osal_stream_buffer_close(compressed);
    return 0;


}
