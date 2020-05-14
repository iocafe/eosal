/**

  @file    defs/common/osal_common_defs.h
  @brief   Micellenous defines common to all operating systems.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  This file contains micellenous defines, like OS_NULL, OS_TRUE..., which are common
  to all operating systems.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/

/** Security and testing is difficult with security on, define to turn much of it off.
 */
#ifndef EOSAL_RELAX_SECURITY
  #define EOSAL_RELAX_SECURITY 0
#endif

/** Null pointer.
 */
#ifndef OS_NULL
#define OS_NULL 0
#endif

/** Value indication boolead condition TRUE.
 */
#define OS_TRUE 1

/** Value indication boolead condition FALSE.
 */
#define OS_FALSE 0

/** Boolean type
 */
typedef os_char os_boolean;

/** Timer value type
 */
#if OSAL_TIMER_IS_64_BITS
#define os_timer os_int64
#else
#define os_timer os_uint
#endif

/** Default OS path buffer size in bytes.
 */
#ifndef OSAL_PATH_SZ
#define OSAL_PATH_SZ 128
#endif

/** C++ compilation: The OSAL_C_HEADER_BEGINS marks beginning of header file text,
    which is to be left undecorated. Undecorated code is used to call C from inside
    C++ code. If compiling a C file, the OSAL_C_HEADER_BEGINS does nothing.
 */
#ifndef OSAL_C_HEADER_BEGINS
#ifdef __cplusplus
#define OSAL_C_HEADER_BEGINS extern "C" {
#else
#define OSAL_C_HEADER_BEGINS
#endif
#endif

/** C++ compilation: The OSAL_C_HEADER_ENDS marks beginning of header file text,
    which is to be left undecorated. See OSAL_C_HEADER_BEGINS. If compiling
    a C file, the OSAL_C_HEADER_ENDS does nothing.
 */
#ifndef OSAL_C_HEADER_ENDS
#ifdef __cplusplus
#define OSAL_C_HEADER_ENDS }
#else
#define OSAL_C_HEADER_ENDS
#endif
#endif

/** IN PC testing we can be more free about memory and include quite a bit of debug information
    when OS_DEBUG is set. On microcontroller we need to be more conservative even when debugging.
    The OSAL_PC_DEBUG define is used to check wether to include extra debug code for PC testing.
 */
#if OSAL_MICROCONTROLLER
#define OSAL_PC_DEBUG 0
#else
#define OSAL_PC_DEBUG OSAL_DEBUG
#endif

/** If we have no interrupt handler function type for this operating system,
    define empty OS_ISR_FUNC_ATTR. Define OS_FLASH_MEM is used to force data
    to flash.
 */
#ifndef OS_ISR_FUNC_ATTR
#define OS_ISR_FUNC_ATTR
#endif
#ifndef OS_ISR_DATA_ATTR
#define OS_ISR_DATA_ATTR
#endif
#ifndef OS_FLASH_MEM
#define OS_FLASH_MEM const
#endif
#ifndef OS_FLASH_MEM_H
#define OS_FLASH_MEM_H const
#endif

/** By default, do not maintain list of function pointers to enable/disable
    all application interrupts by one function call.
 */
#ifndef OSAL_INTERRUPT_LIST_SUPPORT
#define OSAL_INTERRUPT_LIST_SUPPORT 0
#endif

/** Maximum number of error handlers. Define 0 to disable error handlers.
 */
#ifndef OSAL_MAX_ERROR_HANDLERS
#define OSAL_MAX_ERROR_HANDLERS 3
#endif

/** Enumeration of bitmap format and color flag. Bitmap format enumeration value is number
    of bits per pixel, with OSAL_BITMAP_COLOR_FLAG (0x100) to indicate color or
    OSAL_BITMAP_ALPHA_CHANNEL_FLAG (0x200) to indicate that bitmap has alpha channel.
 */
#define OSAL_BITMAP_COLOR_FLAG 0x80
#define OSAL_BITMAP_ALPHA_CHANNEL_FLAG 0x40
#define OSAL_BITMAP_BYTES_PER_PIX(f) (((os_int)(f) & 0x3F) >> 3)
typedef enum osalBitmapFormat
{
    OSAL_GRAYSCALE8 = 8,
    OSAL_GRAYSCALE16 = 16,
    OSAL_RGB24 = 24 | OSAL_BITMAP_COLOR_FLAG,
    OSAL_RGBA32 = 32  | OSAL_BITMAP_COLOR_FLAG | OSAL_BITMAP_ALPHA_CHANNEL_FLAG
}
osalBitmapFormat;

/** Define either 0 or 1 depending if we use RGB or BGR color order in internal bitmaps.
 */
#define OSAL_BGR_COLORS 0

/** Default task stack sizes in bytes. These can be overridden for operating system or for
    a spacific build.
 */
#ifndef OSAL_THREAD_SMALL_STACK
#define OSAL_THREAD_SMALL_STACK 4096
#endif
#ifndef OSAL_THREAD_NORMAL_STACK
#define OSAL_THREAD_NORMAL_STACK 8192
#endif
#ifndef OSAL_THREAD_LARGE_STACK
#define OSAL_THREAD_LARGE_STACK 16384
#endif
