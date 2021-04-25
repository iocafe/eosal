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
#pragma once
#ifndef OSAL_COMMON_DEFS_H_
#define OSAL_COMMON_DEFS_H_
#include "eosal.h"

/* EOSAL library version number.
 */
#ifndef EOSAL_VERSION
#define EOSAL_VERSION "210424"
#endif

/* Get version (build date and time) and set X's if we do not have these
 */
#if defined __has_include
#if __has_include ("eosal_version.h")
#include "eosal_version.h"
#endif
#endif
#ifndef OSAL_BUILD_DATE
    #define OSAL_BUILD_DATE "XXXXXX"
#endif
#ifndef OSAL_BUILD_TIME
    #define OSAL_BUILD_TIME "XXXX"
#endif
#ifndef OSAL_BUILD_DATETIME
    #define OSAL_BUILD_DATETIME "XXXXXX-XXXX"
#endif

/** Security and testing is difficult with security on, define to turn much of it off.
 */
#ifndef OSAL_RELAX_SECURITY
  #define OSAL_RELAX_SECURITY 0
#endif

/** Usually we do not check server certificate expiration (under no circumstances
    we want out automation system to go down at specific date). In internet applications
    for general public, this may be desirable and OSAL_CHECK_SERVER_CERT_EXPIRATION can
    be defined as 1.
 */
#ifndef OSAL_CHECK_SERVER_CERT_EXPIRATION
#define OSAL_CHECK_SERVER_CERT_EXPIRATION 0
#endif

/** Null pointer.
 */
#ifndef OS_NULL
#ifdef __cplusplus
#define OS_NULL NULL
#else
#define OS_NULL 0
#endif
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

/** OSAL extensions: If OSAL_STREAM_BUFFER_SUPPORT is not defined in osal_defs.h
    or in build flags, decide if it needed by other configuration flags.
    For Windows and Linux this should be always 1, this is only for microcontroller.
 */
#ifndef OSAL_STREAM_BUFFER_SUPPORT
#if OSAL_JSON_TEXT_SUPPORT
#define OSAL_STREAM_BUFFER_SUPPORT 1
#endif
#endif

#ifndef OSAL_STREAM_BUFFER_SUPPORT
#if OSAL_MAIN_SUPPORT
#define OSAL_STREAM_BUFFER_SUPPORT 1
#endif
#endif

#ifndef OSAL_STREAM_BUFFER_SUPPORT
#if OSAL_DYNAMIC_MEMORY_ALLOCATION
#define OSAL_STREAM_BUFFER_SUPPORT 1
#endif
#endif

#ifndef OSAL_STREAM_BUFFER_SUPPORT
#define OSAL_STREAM_BUFFER_SUPPORT 0
#endif

#ifndef OSAL_RING_BUFFER_SUPPORT
#define OSAL_RING_BUFFER_SUPPORT OSAL_SOCKET_SUPPORT
#endif

/* Decide if to include nick name support. By default we support nick names
 * we have sockets and we are not doing minimalistic build.
 */
#ifndef OSAL_NICKNAME_SUPPORT
  #if OSAL_MINIMALISTIC == 0
    #if OSAL_SOCKET_SUPPORT
      #define OSAL_NICKNAME_SUPPORT 1
    #endif
  #endif
#endif
#ifndef OSAL_NICKNAME_SUPPORT
  #define OSAL_NICKNAME_SUPPORT 0
#endif

/** By default we do not have unique identifier of CPU.
 */
#ifndef OSAL_CPUID_SUPPORT
#define OSAL_CPUID_SUPPORT 0
#endif

/** By default we do use AES encryption for secrets if we have TLS.
 */
#ifndef OSAL_AES_CRYPTO_SUPPORT
#define OSAL_AES_CRYPTO_SUPPORT OSAL_TLS_SUPPORT
#endif

/** By default: We use CPUID in secret crypt key, if this is we have CPUID support and
    this is microcontroller. We do not use this in PC, since we want to be able to make
    a working backup of PC server and run it in spare computer.
 */
#ifndef OSAL_AES_CRYPTO_WITH_CPUID
#define OSAL_AES_CRYPTO_WITH_CPUID (OSAL_AES_CRYPTO_SUPPORT && OSAL_CPUID_SUPPORT && OSAL_MICROCONTROLLER)
#endif


/** Support for operating system event lists.
 */
#define OSAL_OS_EVENT_LIST_SUPPORT (OSAL_PROCESS_CLEANUP_SUPPORT && OSAL_MULTITHREAD_SUPPORT)

/** Define OSAL_DEBUG_FILE_AND_LINE as 1 to include file name and line number in osal_debug
    and osal_assert macros. Effective only when OSAL_DEBUG is 1.
 */
#ifndef OSAL_DEBUG_FILE_AND_LINE
#define OSAL_DEBUG_FILE_AND_LINE (OSAL_MINIMALISTIC == 0)
#endif


/** Macro to flag unused function argument so that compiler warning "unused parameter"
    is not generated.
 */
#define OSAL_UNUSED(x) (void)(x)

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

/** If operating system name is unspecified, default to bin directory name.
    (often same as operating system name)
 */
#ifndef OSAL_OS_NAME
#define OSAL_OS_NAME OSAL_BIN_NAME
#endif


/** If operating system name is unspecified, default to bin directory name.
    (often same as operating system name)
 */
#ifndef OSAL_OSVER
#define OSAL_OSVER "generic"
#endif

/** If architecture is not specified, default to "generic".
 */
#ifndef OSAL_ARCH
#define OSAL_ARCH "generic"
#endif

/** If we have no interrupt handler function type for this operating system,
    define empty OS_ISR_FUNC_ATTR.
 */
#ifndef OS_ISR_FUNC_ATTR
#define OS_ISR_FUNC_ATTR
#endif
#ifndef OS_ISR_DATA_ATTR
#define OS_ISR_DATA_ATTR
#endif

/** Depending on hardware, we can use OS_PROGMEM attribute to place data on flash only (no RAM copy),
 *  but may need to access it trough os_memcpy_P() function.
 *  OS_CONST define is used if our hardware allows us to place const data on flash, which can
 *  be used directly trough pointer.
 *  Define with "_H" suffix  is meant for .h file, like "extern OS_CONST_H os_int my_int;" and the
 *  other for C file (for example "OS_CONST os_int my_int = 1234;"
 *  The general purpose defines here just state that data is constant without any HW specific attributes.
 *  Flag IOC_STATIC_MBLK_IN_PROGMEN instructs IOCOM that static IO configuration is kept
 *  in flash memory, and needs to be copied with os_memcpy_P (off for generic HW).
 */
#ifndef OS_CONST
#define OS_CONST const
#endif
#ifndef OS_CONST_H
#define OS_CONST_H const
#endif
#ifndef OS_PROGMEM
#define OS_PROGMEM const
#endif
#ifndef OS_PROGMEM_H
#define OS_PROGMEM_H const
#endif
#ifndef IOC_STATIC_MBLK_IN_PROGMEN
#define IOC_STATIC_MBLK_IN_PROGMEN 0
#endif

/** Do we need support for minimum - maximum ranges for integer types?
    By default we support these on PC, not on microcontroller.
 */
#define OSAL_TYPE_RANGE_SUPPORT (OSAL_MICROCONTROLLER == 0 && OSAL_TYPEID_SUPPORT)

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
