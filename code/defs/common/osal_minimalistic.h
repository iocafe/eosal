/**

  @file    defs/common/osal_minimalistic.h
  @brief   Defaults for minimalistic eosal/iocom build with serial communcation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    25.4.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/

#ifndef OSAL_SOCKET_SUPPORT
#define OSAL_SOCKET_SUPPORT 0
#endif

#ifndef OSAL_TLS_SUPPORT
#define OSAL_TLS_SUPPORT 0
#endif

#ifndef OSAL_SERIALIZE_SUPPORT
#define OSAL_SERIALIZE_SUPPORT 0
#endif

#ifndef OSAL_SERIAL_SUPPORT
#define OSAL_SERIAL_SUPPORT 1
#endif

#ifndef OSAL_BLUETOOTH_SUPPORT
#define OSAL_BLUETOOTH_SUPPORT 0
#endif

#ifndef OSAL_MULTITHREAD_SUPPORT
#define OSAL_MULTITHREAD_SUPPORT 0
#endif

#ifndef OSAL_PERSISTENT_SUPPORT
#define OSAL_PERSISTENT_SUPPORT 0
#endif

#ifndef OSAL_DEVICE_PROGRAMMING_SUPPORT
#define OSAL_DEVICE_PROGRAMMING_SUPPORT 0
#endif

#ifndef OSAL_PROCESS_SUPPORT
#define OSAL_PROCESS_SUPPORT 0
#endif

#ifndef OSAL_TRACE
#define OSAL_TRACE 0
#endif

#ifndef OSAL_DEBUG
#define OSAL_DEBUG 0
#endif

#ifndef OSAL_MEMORY_DEBUG
#define OSAL_MEMORY_DEBUG 0
#endif

#ifndef OSAL_MAX_ERROR_HANDLERS
#define OSAL_MAX_ERROR_HANDLERS 0
#endif

#ifndef OSAL_UTF8
#define OSAL_UTF8 0
#endif

#ifndef OSAL_TYPEID_SUPPORT
#define OSAL_TYPEID_SUPPORT 0
#endif

#ifndef IOC_DEVICE_STREAMER
#define IOC_DEVICE_STREAMER 0
#endif

#ifndef IOC_CONTROLLER_STREAMER
#define IOC_CONTROLLER_STREAMER 0
#endif

#ifndef IOC_DYNAMIC_MBLK_CODE
#define IOC_DYNAMIC_MBLK_CODE 0
#endif

#ifndef IOC_AUTHENTICATION_CODE
#define IOC_AUTHENTICATION_CODE 0
#endif

