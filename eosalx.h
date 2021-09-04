/**

  @file    eosalx.h
  @brief   Main OSAL header file with extensions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  This operating system abstraction layer (OSAL) base main header file with extensions.
  If further OSAL base and extension headers.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef EOSALX_H_
#define EOSALX_H_

/* Include osal base.
 */
#include "eosal.h"

/* If C++ compilation, all functions, etc. from this point on in this header file are
   plain C and must be left undecorated.
 */
OSAL_C_HEADER_BEGINS

/* Include extension headers.
 */
#if OSAL_TIME_SUPPORT
  #include "extensions/time/common/osal_time.h"
#endif

#if OSAL_RAND_SUPPORT
  #include "extensions/rand/common/osal_rand.h"
#endif

#include "extensions/math/common/osal_round.h"

/* Include osal_main.h header regardless of OSAL_MAIN_SUPPORT define. Needed for
   micro-controller environments.
 */
#include "extensions/main/common/osal_main.h"
#ifdef E_OS_linux
  #include "extensions/main/linux/osal_linux_main.h"
#endif
#ifdef E_OS_windows
  #include "extensions/main/windows/osal_windows_main.h"
#endif
#ifndef EOSAL_C_MAIN
  #define EOSAL_C_MAIN
#endif
#include "extensions/ringbuf/common/osal_ringbuf.h"
#include "extensions/stream/common/osal_stream.h"
#include "extensions/stream/common/osal_stream_defaults.h"
#include "extensions/stream/common/osal_stream_buffer.h"
#include "extensions/stream/common/osal_stream_supplement.h"

#if OSAL_STRINGX_SUPPORT
  #include "extensions/stringx/common/osal_stringx.h"
#endif

#include "extensions/typeid/common/osal_typeid.h"

/* We need to include headers, even if we do not need sockets, tls or serial.
   We need empty placeholder macros, etc. when these are not supportted.
 */
#include "extensions/net/common/osal_dns.h"
#include "extensions/net/common/osal_net_config.h"
#include "extensions/net/common/osal_net_state.h"
#include "extensions/net/common/osal_net_morse_code.h"
#include "extensions/socket/common/osal_socket.h"
#include "extensions/socket/common/osal_socket_util.h"
#include "extensions/tls/common/osal_tls.h"
#include "extensions/tls/common/osal_crypto_hash.h"
#include "extensions/tls/common/osal_aes_crypt.h"
#include "extensions/serial/common/osal_serial.h"
#include "extensions/bluetooth/common/osal_bluetooth.h"

#include "extensions/secret/common/osal_secret.h"

#if OSAL_FILESYS_SUPPORT
  #include "extensions/filesys/common/osal_file.h"
  #include "extensions/filesys/common/osal_dir.h"
  #include "extensions/filesys/common/osal_filestat.h"
  #include "extensions/filesys/common/osal_filectrl.h"
#endif

#include "extensions/filesys/common/osal_fileutil.h"

#if OSAL_SERIALIZE_SUPPORT
  #include "extensions/serialize/common/osal_serialize.h"
  #include "extensions/serialize/common/osal_json_shared.h"
  #include "extensions/serialize/common/osal_json_indexer.h"
#endif

#if OSAL_JSON_TEXT_SUPPORT
  #include "extensions/serialize/common/osal_compress_json.h"
  #include "extensions/serialize/common/osal_uncompress_json.h"
#endif

#if OSAL_PERSISTENT_SUPPORT
  #include "extensions/persistent/common/osal_persistent.h"
#endif

#include "extensions/program/common/osal_program_device.h"
#include "extensions/checksum/common/osal_checksum.h"

#if OSAL_PROCESS_SUPPORT
  #include "extensions/process/common/osal_process.h"
#endif

#if OSAL_CPUID_SUPPORT
  #include "extensions/cpuid/common/osal_cpuid.h"
#endif

#if OSAL_USE_JPEG_LIBRARY
  #include "extensions/jpeg/common/eosal_jpeg.h"
#endif


/* If C++ compilation, end the undecorated code.
 */
OSAL_C_HEADER_ENDS

#endif
