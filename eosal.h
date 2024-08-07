/**

  @file    eosal.h
  @brief   Main OSAL header file.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  This operating system abstraction layer (OSAL) base main header file. If further includes
  rest of OSAL base headers.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef EOSAL_H_
#define EOSAL_H_

/* Include definitions common to all operating systems (before OS specific defines).
 */
#include "code/defs/common/osal_common_pre_defs.h"

/* Include target operating system specific defines.
 */
#ifdef E_OS_esp32
  #include "code/defs/esp32/osal_defs.h"
  #include "code/defs/esp32/osal_types.h"
  #define EOSAL_TARGET_HDRS_INCLUDED
#endif

#ifdef E_OS_arduino
  #include "code/defs/duino/osal_defs.h"
  #include "code/defs/duino/osal_types.h"
  #define EOSAL_TARGET_HDRS_INCLUDED
#endif

#ifdef E_OS_linux
  #include "code/defs/linux/osal_defs.h"
  #include "code/defs/linux/osal_types.h"
  #define EOSAL_TARGET_HDRS_INCLUDED
#endif

#ifdef E_OS_windows
  #include "code/defs/windows/osal_defs.h"
  #include "code/defs/windows/osal_types.h"
  #define EOSAL_TARGET_HDRS_INCLUDED
#endif

#ifdef E_OS_metal
  #include "code/defs/metal/osal_defs.h"
  #include "code/defs/metal/osal_types.h"
  #define EOSAL_TARGET_HDRS_INCLUDED
#endif

/* If target operating system is unspecified, include headers by build environment.
 */
#ifndef EOSAL_TARGET_HDRS_INCLUDED
  #ifdef ESP_PLATFORM
    #include "code/defs/esp32/osal_defs.h"
    #include "code/defs/esp32/osal_types.h"
    #define EOSAL_TARGET_HDRS_INCLUDED
    #define E_OS_esp32
  #endif
#endif

#ifndef EOSAL_TARGET_HDRS_INCLUDED
  #ifdef ARDUINO
    #include "code/defs/duino/osal_defs.h"
    #include "code/defs/duino/osal_types.h"
    #define EOSAL_TARGET_HDRS_INCLUDED
    #define E_OS_arduino
  #endif
#endif

#ifndef EOSAL_TARGET_HDRS_INCLUDED
  #ifdef _WIN32
    #include "code/defs/windows/osal_defs.h"
    #include "code/defs/windows/osal_types.h"
    #define EOSAL_TARGET_HDRS_INCLUDED
    #define E_OS_windows
  #endif
#endif

#ifndef EOSAL_TARGET_HDRS_INCLUDED
  #include "code/defs/linux/osal_defs.h"
  #include "code/defs/linux/osal_types.h"
  #define E_OS_linux
#endif


/* Include definitions common to all operating systems (after OS specific defines).
 */
#include "code/defs/common/osal_common_post_defs.h"

/* If C++ compilation, all functions, etc. from this point on in this header file are
   plain C and must be left undecorated.
 */
OSAL_C_HEADER_BEGINS

/* Include generic OSAL headers.
 */
#include "code/int64/common/osal_int64.h"
#include "code/memory/common/osal_memory.h"
#include "code/memory/common/osal_sysmem.h"
#include "code/memory/common/osal_psram.h"
#include "code/string/common/osal_char.h"
#include "code/defs/common/osal_status.h"
#include "code/string/common/osal_string.h"
#include "code/defs/common/osal_state_bits.h"
#include "code/timer/common/osal_timer.h"
#include "code/error/common/osal_error.h"
#include "code/debugcode/common/osal_debug.h"
#include "code/resmon/common/osal_resource_monitor.h"
#include "code/console/common/osal_console.h"
#include "code/console/common/osal_sysconsole.h"
#include "code/event/common/osal_event.h"
#include "code/defs/common/osal_global.h"
#include "code/initialize/common/osal_initialize.h"
#include "code/mutex/common/osal_mutex.h"
#include "code/mutex/common/osal_interrupt_list.h"
#include "code/thread/common/osal_thread.h"
#include "code/utf16/common/osal_utf16.h"
#include "code/utf32/common/osal_utf32.h"

/* If C++ compilation, end the undecorated code.
 */
OSAL_C_HEADER_ENDS

#endif
