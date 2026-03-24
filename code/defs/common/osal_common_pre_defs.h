/**

  @file    defs/common/osal_common_pre_defs.h
  @brief   Micellenous defines common to all operating systems (before OS specific osal_defs.h).
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  This file contains micellenous defines, like OS_NULL, OS_TRUE..., which are common
  to all operating systems.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_COMMON_PRE_DEFS_H_
#define OSAL_COMMON_PRE_DEFS_H_
#include "eosal.h"

/** Value indication boolead condition TRUE.
 */
#define OS_TRUE 1

/** Value indication boolead condition FALSE.
 */
#define OS_FALSE 0

/** Timeout to wait for forever.
 */
#define OSAL_INFINITE -1

/** Constants for selecting persistent support.
 */
#define OSAL_PERSISTENT_DEFAULT_STORAGE 1
#define OSAL_PERSISTENT_EEPROM_STORAGE 2
#define OSAL_PERSISTENT_NVS_STORAGE 3

/** Socket library enumeration (OS/Platform dependent).
 */
#define OSAL_NO_NETWORK_INTERFACE 0
#define OSAL_OS_NETWORK_INTERFACE 1
#define OSAL_WIZ_NETWORK_INTERFACE 2
#define OSAL_LWIP_NETWORK_INTERFACE 3
#define OSAL_SAM_NETWORK_INTERFACE 4
#define OSAL_NETCONN_NETWORK_INTERFACE 5

/** Possible values for OSAL_TLS_SUPPORT (TLS wrapper implementation).
 */
#define OSAL_TLS_NONE 0
#define OSAL_TLS_OPENSSL_WRAPPER 1
#define OSAL_TLS_MBED_WRAPPER 2
#define OSAL_TLS_ARDUINO_WRAPPER 3
#define OSAL_TLS_SAM_WRAPPER 4

/** Enumeration of bitmap format and color flag. Bitmap format enumeration value is number
    of bits per pixel, with OSAL_BITMAP_COLOR_FLAG (0x100) to indicate color or
    OSAL_BITMAP_ALPHA_CHANNEL_FLAG (0x200) to indicate that bitmap has alpha channel.
    Notice that the defined format values should not be changed, hard coded in egui.
 */
#define OSAL_BITMAP_COLOR_FLAG 0x80
#define OSAL_BITMAP_ALPHA_CHANNEL_FLAG 0x40
#define OSAL_BITMAP_BYTES_PER_PIX(f) (((os_int)(f) & 0x3F) >> 3)
typedef enum osalBitmapFormat
{
    OSAL_BITMAP_FORMAT_NOT_SET = 0,
    OSAL_GRAYSCALE8 = 8,
    OSAL_GRAYSCALE16 = 16,
    OSAL_RGB24 = 24 | OSAL_BITMAP_COLOR_FLAG,
    OSAL_RGB32 = 32 | OSAL_BITMAP_COLOR_FLAG,
    OSAL_RGBA32 = 32 | OSAL_BITMAP_COLOR_FLAG | OSAL_BITMAP_ALPHA_CHANNEL_FLAG
}
osalBitmapFormat;

/** Define either 0 or 1 depending if we use RGB or BGR color order in internal bitmaps.
 */
#define OSAL_BGR_COLORS 0

#endif
