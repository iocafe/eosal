/**

  @file    defs/metal/osal_defs.h
  @brief   Operating system specific defines for Linux.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  This file contains platform specific defines for windows compilation. The platform specific
  defines here are defaults, which can be overwritten by compiler settings.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eobjects project and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_DEFS_H_
#define OSAL_DEFS_H_

/** Operating system identifier define. There is define for each supported operating system,
    for example OEAL_WINDOWS, OSAL_LINUX, OSAL_ARDUINO, OSAL_METAL... Compilation can be conditioned
    by the define, for example "#ifdef OSAL_LINUX".
 */
#define OSAL_METAL 1

/* If we want the default defines for a minimalistic serial communication device.
 */
#ifndef OSAL_MINIMALISTIC
  #define OSAL_MINIMALISTIC 0
#endif
#if OSAL_MINIMALISTIC
  #include "code/defs/common/osal_minimalistic.h"
#endif

#ifdef OSAL_INCLUDE_METAL_HEADERS
#ifdef STM32L476xx
#include "stm32l4xx_hal.h"
#endif
#endif

/** Default level for OS which osal_trace() lines to compile in. OSAL_DEBUG required to trace.
 */
#ifndef OSAL_TRACE
#define OSAL_TRACE 1
#endif

/** C compiler's debug build flag.
 */
#ifndef OSAL_CC_DEBUG
  #ifdef NDEBUG
    #define OSAL_CC_DEBUG 0
  #else
    #define OSAL_CC_DEBUG 1
  #endif
#endif

/** Generic include debug code in compilation. The debug code checks for programming errors.
 */
#ifndef OSAL_DEBUG
#define OSAL_DEBUG 1
#endif

/** Define 1 is this is micro-controller environment. This is used to get sensible default
    defines for some features.
 */
#ifndef OSAL_MICROCONTROLLER
#define OSAL_MICROCONTROLLER 1
#endif

/** Byte order. If OSAL_SMALL_ENDIAN flags is nonzero, the small endian byte format is assumed,
    otherwise big endian.
 */
#ifndef OSAL_SMALL_ENDIAN
#define OSAL_SMALL_ENDIAN 1
#endif

/** Set if dynamic memory allocation is to be supported. This is off only when using OSAL
    for microcontrollers or other systems with very limited memory resources.
 */
#ifndef OSAL_DYNAMIC_MEMORY_ALLOCATION
#define OSAL_DYNAMIC_MEMORY_ALLOCATION 1
#endif

/** Include memory debug code. If OSAL_MEMORY_DEBUG flags is nonzero, the memory
    block overflows and freed block size are checked.
 */
#ifndef OSAL_MEMORY_DEBUG
#define OSAL_MEMORY_DEBUG 1
#endif

/** OSAL memory allocation manager flag. If OSAL_MEMORY_MANAGER flags is nonzero, then
    memory is allocated through OSAL memory management. If this flag is zero, then
    operating system memory allocation is called directly.
 */
#ifndef OSAL_MEMORY_MANAGER
#define OSAL_MEMORY_MANAGER 1
#endif

/** Include resource monitor code. If OSAL_RESOURCE_MONITOR flags is nonzero, code for
    monitoring operating system resource use is included.
 */
#ifndef OSAL_RESOURCE_MONITOR
#if OSAL_DEBUG
#define OSAL_RESOURCE_MONITOR 1
#else
#define OSAL_RESOURCE_MONITOR 0
#endif
#endif

/** Include code to force os_lock() to switch to time critical priority
    when in system mutex is locked. This can be used on systems which do not support
    priority inheritance to avoid priority reversal, like desktop windows. Anyhow setting
    this option slows the code down. On systems which do support priority inheritance,
    like Windows mobile, ThreadX, etc. this should be set to zero.
 */
#ifndef OSAL_TIME_CRITICAL_SYSTEM_LOCK
#define OSAL_TIME_CRITICAL_SYSTEM_LOCK 0
#endif

/** Include UTF8 character encoding support, define 1 or 0. For most systems UTF8 character
    encoding should be enabled. Disale UTF8 only for very minimalistic systems which need
    only English strings and where every extra byte counts.
 */
#ifndef OSAL_UTF8
#define OSAL_UTF8 1
#endif

 /** Include UTF16 character encoding support, define 1 or 0. For most systems UTF16
     character encoding should be disabled. Enable UTF16 for Windows Unicode support.
     Notice that OSAL_UTF8 must be enbaled to enable OSAL_UTF16.
 */
#ifndef OSAL_UTF16
#define OSAL_UTF16 0
#endif

/** Enable or disable console support.
 */
#ifndef OSAL_CONSOLE
#define OSAL_CONSOLE 1
#endif

/** A short name for operating system/platform. For example "windows", "linux",
    "arduino", "metal"...
    This is needed for cross compiling to multiple processor architectures:
    We need to maintain separate compiled binaries, libraries and and intermediate
    files. If necessary, there there can be multiple names for same operating system.
 */
#ifndef OSAL_BIN_NAME
#define OSAL_BIN_NAME "metal"
#endif

/** Default file system root. This is path to default root of the file system.
 */
#ifndef OSAL_FS_ROOT
#define OSAL_FS_ROOT "/"
#endif

/** Needed memory alignment. Some processors require that variables are allocated at
    type size boundary. For example ARMv6 requires that 2 byte integers starts from
    pair addressess and 4 byte integers from addressess dividable by four. If so,
    define "OSAL_MEMORY_TYPE_ALIGNMENT 4". If no aligment is needed define 0 here.

    ARM: Hardware support for unaligned accesses was first introduced in ARMv6.
    Cortex-M4(F) is based on ARMv7-M and supports unaligned accesses.
    ARMv4/v4T is an older architecture and devices like LPC2148 which are based
    on ARM7TDMI-S don't support unaligned accesses.

    Copying the 32-bit data byte by byte will need 4 memory read cycles.
    The broken data is then reassembled using shifts and OR operations.

    We use 4 byte alignment here because it works on all ARM32 versions and
    has performance benefit over nonaligned memory.
 */
#ifndef OSAL_MEMORY_TYPE_ALIGNMENT
#define OSAL_MEMORY_TYPE_ALIGNMENT 4
#endif


/** If compiler supports 64 bit integer type, like "__int64" or "long long" or plain
   "long" in 64 bit GNU compilers. Define 0 if compiler doesn't have 64 bit support.
 */
#ifndef OSAL_COMPILER_HAS_64_BIT_INTS
#define OSAL_COMPILER_HAS_64_BIT_INTS 1
#endif

/** Select is os_long or or 64 bit. Typically 1 for 32/64 bit processors and 0 for 8/16 bit processors.
    If set, os_long is 64 bit type, like "__int64" or "long long" or plain "long" in 64 bit GNU compilers.
    Define 0 if compiler doesn't have 64 bit support.
 */
#ifndef OSAL_LONG_IS_64_BITS
#define OSAL_LONG_IS_64_BITS 0
#endif

/** Timer is 64 bit integer. Define 1 if timer value type is 64 bit integer. If 0, then
    32 bit timer type is used.
 */
#ifndef OSAL_TIMER_IS_64_BITS
#define OSAL_TIMER_IS_64_BITS 0
#endif

/** OSAL proces cleanup code needed flag. If OSAL_PROCESS_CLEANUP_SUPPORT flags is nonzero,
    then code to do memory, etc. cleanup when process exists or restarts is included.
    If this flag is zero, then cleanup code is not included. The cleanup code should
    be included in platforms like Windows or Linux, where processes can be terminated
    or restarted. Memory cleanup code is not necessary on most of embedded systems,
    disabling it saves a few bytes of memory.
 */
#ifndef OSAL_PROCESS_CLEANUP_SUPPORT
#define OSAL_PROCESS_CLEANUP_SUPPORT 0
#endif

/** Multithreading support. Define 1 if operating system supports multi threading. This
    enables code for thread, mutexes, event, etc. Define 0 if there is no multithreading
    support for this operating system.
 */
#ifndef OSAL_MULTITHREAD_SUPPORT
#define OSAL_MULTITHREAD_SUPPORT 0
#endif

/** Socket options for the platform
 */
#define OSAL_SOCKET_SELECT_SUPPORT 0

/** Socket maintain support is something typical to single thread mode in
    micro controllers. For those we may need to call a maintain function
    periodically to keep up with DHCP leases, etc.
 */
#define OSAL_SOCKET_MAINTAIN_NEEDED 1

/* Enumeration of socket types.
 */
#define OSAL_SOCKET_NONE 0
#define OSAL_SOCKET_AUTO_SELECT 1
#define OSAL_SOCKET_WIZNET 2
#define OSAL_SOCKET_LWIP 3

/* If socket support if not selected by compiler define, select now.
 * Socket support can be selected like "/DOSAL_SOCKET_SUPPORT=3"
 */
#ifdef OSAL_SOCKET_SUPPORT
  #if OSAL_SOCKET_SUPPORT==OSAL_SOCKET_AUTO_SELECT
    #undef OSAL_SOCKET_SUPPORT
  #endif
#endif
#ifndef OSAL_SOCKET_SUPPORT
  #ifdef STM32L4xx
    #define OSAL_SOCKET_SUPPORT OSAL_SOCKET_WIZNET
  #endif
  #ifdef STM32F4xx
    #define OSAL_SOCKET_SUPPORT OSAL_SOCKET_LWIP
  #endif
#endif

/** Include code for static IP configuration?
 */
#ifndef OSAL_SUPPORT_STATIC_NETWORK_CONF
#define OSAL_SUPPORT_STATIC_NETWORK_CONF OSAL_SOCKET_SUPPORT
#endif

/** Include code for MAC address configuration ?
 */
#ifndef OSAL_SUPPORT_MAC_CONF
#define OSAL_SUPPORT_MAC_CONF 0
#endif

/** Include code for WiFI network configuration?
 */
#ifndef OSAL_SUPPORT_WIFI_NETWORK_CONF
#define OSAL_SUPPORT_WIFI_NETWORK_CONF 0
#endif

/* Unknown micro controller build, default to WizNET chip.
 */
#ifndef OSAL_SOCKET_SUPPORT
  #define OSAL_SOCKET_SUPPORT OSAL_SOCKET_WIZNET
#endif


/** Select TLS wrapper implementation to use.
 */
#ifndef OSAL_TLS_SUPPORT
#define OSAL_TLS_SUPPORT OSAL_TLS_NONE
#endif

/** If serial communication is supported for the platform, define 1.
 */
#ifndef OSAL_SERIAL_SUPPORT
#define OSAL_SERIAL_SUPPORT 1
#endif

/** Serial communication options for the bare metal
 */
#ifndef OSAL_SERIAL_SELECT_SUPPORT
#define OSAL_SERIAL_SELECT_SUPPORT 0
#endif

/** If bluetooth serial communication is supported and to be included, define 1.
 */
#ifndef OSAL_BLUETOOTH_SUPPORT
#define OSAL_BLUETOOTH_SUPPORT 0
#endif

/** If low level serialization is supported for the platform, define 1.
 */
#ifndef OSAL_SERIALIZE_SUPPORT
#define OSAL_SERIALIZE_SUPPORT 0
#endif

/** Support parsing and writing JSON as plain text. Compressed binary JSON support
 *  is already included in OSAL_SERIALIZE_SUPPORT.
 */
#ifndef OSAL_JSON_TEXT_SUPPORT
#define OSAL_JSON_TEXT_SUPPORT 0
#endif

/** OSAL extensions: If file system is supported for the platform, define 1.
 */
#ifndef OSAL_FILESYS_SUPPORT
#define OSAL_FILESYS_SUPPORT 0
#endif

/** OSAL extensions: If 64 bit integer multiplication, division or
    to/from double conversion are needed without compiler's 64 bit
    support (OSAL_COMPILER_HAS_64_BIT_INTS is zero), then define 1.
    If OSAL_COMPILER_HAS_64_BIT_INTS is 1, this flag is ignored.
 */
#ifndef OSAL_INT64X_SUPPORT
#define OSAL_INT64X_SUPPORT 0
#endif

/** OSAL extensions: If using osal_main() as entry point is supported for
    the platform, define 1.
 */
#ifndef OSAL_MAIN_SUPPORT
#define OSAL_MAIN_SUPPORT 0
#endif

/** OSAL extensions: If osal_rand() is supported for the platform, define 1.
 */
#define OSAL_RAND_COMMON 1
#define OSAL_RAND_PLATFORM 2
#ifndef OSAL_RAND_SUPPORT
#define OSAL_RAND_SUPPORT OSAL_RAND_PLATFORM
#endif

/** OSAL extensions: If extended string functions are supported
    for the platform, define 1.
 */
#ifndef OSAL_STRINGX_SUPPORT
#define OSAL_STRINGX_SUPPORT 1
#endif

/** OSAL extensions: If GMT clock is supported for the platform, define 1.
 */
#ifndef OSAL_TIME_SUPPORT
#define OSAL_TIME_SUPPORT 0
#endif

/** OSAL extensions: If type identifiers are supported for the platform, define 1.
 */
#ifndef OSAL_TYPEID_SUPPORT
#define OSAL_TYPEID_SUPPORT 0
#endif

/** OSAL extensions: Define 0 if there is no perststant storage support. Define 1 to use the
    default persistant storage implementation for the platform. Values >= 2 select alternate
    persistant storage implementations, for example if we have EEPROM chip on STM32F407 board.
    The alternate implementation numbers are platform specific and the C implementations may
    be board specific.
 */
#ifndef OSAL_PERSISTENT_SUPPORT
#define OSAL_PERSISTENT_SUPPORT 0
#endif

/** OSAL extensions: Do we support secret for device identification and encrypting
    local confidential data.
 */
#ifndef OSAL_SECRET_SUPPORT
#define OSAL_SECRET_SUPPORT OSAL_PERSISTENT_SUPPORT
#endif

/** Do we have flash programming support?
 */
#ifndef OSAL_DEVICE_PROGRAMMING_SUPPORT
#define OSAL_DEVICE_PROGRAMMING_SUPPORT 0
#endif

/** Can we create new processes?
 */
#ifndef OSAL_PROCESS_SUPPORT
#define OSAL_PROCESS_SUPPORT 0
#endif

/** Having a console for testing makes sense in linux and windows PC environments,
    and may be used also in micro-controller systems trough serial port.
 */
#ifndef OSAL_CONTROL_CONSOLE_SUPPORT
#define OSAL_CONTROL_CONSOLE_SUPPORT 0
#endif

#endif
