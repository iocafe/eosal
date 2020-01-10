/**

  @file    osal_compress_json.h
  @brief   Compress JSON as binary data.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#if OSAL_JSON_TEXT_SUPPORT

/* Flags
 */
#define OSAL_JSON_SIMPLIFY 0
#define OSAL_JSON_KEEP_QUIRKS 1

/* Compress JSON from normal string presentation to binary format.
 */
osalStatus osal_compress_json(
    osalStream compressed,
    const os_char *json_source,
    const os_char *skip_tags,
    os_int flags);

#endif
