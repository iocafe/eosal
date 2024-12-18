/**

  @file    osal_uncompress_json.h
  @brief   Uncompress JSON from binary data.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_UNCOMPRESS_JSON_H_
#define OSAL_UNCOMPRESS_JSON_H_
#include "eosalx.h"

#if OSAL_JSON_TEXT_SUPPORT

/* Uncompress JSON from binary data to plain text.
 */
osalStatus osal_uncompress_json(
    osalStream uncompressed,
    os_char *compressed,
    os_memsz compressed_sz,
    os_int flags);

#endif
#endif
