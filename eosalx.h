/**

  @file    eosalx.h
  @brief   Main OSAL header file with extensions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This operating system abstraction layer (OSAL) base main header file with extensions. 
  If further OSAL base and extension headers. 

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_EXTENDED_INCLUDED
#define OSAL_EXTENDED_INCLUDED

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

/* Include osal_main.h header regardless of OSAL_MAIN_SUPPORT define. Needed for micro controller
   environments.
 */
#include "extensions/main/common/osal_main.h"

#if OSAL_STRCONV_SUPPORT
  #include "extensions/strcnv/common/osal_strcnv.h"
#endif

#include "extensions/stream/common/osal_stream.h"
#include "extensions/stream/common/osal_stream_buffer.h"

#if OSAL_STRINGX_SUPPORT
  #include "extensions/stringx/common/osal_stringx.h"
#endif

#if OSAL_TYPEID_SUPPORT
  #include "extensions/typeid/common/osal_typeid.h"
#endif

/* We need to include headers, even if we do not need sockets, tls or serial.
   We need empty placeholder macros when these are not supportted.
 */
#include "extensions/socket/common/osal_socket.h"
#include "extensions/tls/common/osal_tls.h"
#include "extensions/serial/common/osal_serial.h"
#include "extensions/bluetooth/common/osal_bluetooth.h"

#if OSAL_FILESYS_SUPPORT
  #include "extensions/filesys/common/osal_file.h"
  #include "extensions/filesys/common/osal_dir.h"
  #include "extensions/filesys/common/osal_filestat.h"
  #include "extensions/filesys/common/osal_fileutil.h"
#endif

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

#include "extensions/checksum/common/osal_checksum.h"

/* If C++ compilation, end the undecorated code.
 */
OSAL_C_HEADER_ENDS

#endif
