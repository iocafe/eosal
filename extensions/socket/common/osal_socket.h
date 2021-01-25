/**

  @file    socket/common/osal_socket.h
  @brief   OSAL stream API for sockets.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Socket speficic function prototypes and definitions to implement OSAL stream API for sockets.
  OSAL stream API is abstraction which makes streams (including sockets) look similar to upper
  levels of code, regardless of operating system or network library implementation.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_SOCKET_H_
#define OSAL_SOCKET_H_
#include "eosalx.h"

/** Stream interface structure for sockets.
 */
#if OSAL_SOCKET_SUPPORT
extern OS_CONST_H osalStreamInterface osal_socket_iface;
#endif

/* Default socket port number for IOCOM.
 */
#define IOC_DEFAULT_SOCKET_PORT 6368
#define IOC_DEFAULT_SOCKET_PORT_STR "6368"

/* Maximum number of socket streams to pass as an argument to osal_socket_select().
 */
#define OSAL_SOCKET_SELECT_MAX 8

#if OSAL_SOCKET_SUPPORT

/** Define to get socket interface pointer. The define is used so that this can
    be converted to function call.
 */
#define OSAL_SOCKET_IFACE &osal_socket_iface

/* Socket library initialized flag.
 */
extern os_boolean osal_sockets_initialized;


/**
****************************************************************************************************

  @name OSAL Socket Functions.

  These functions implement sockets as OSAL stream. These functions can either be called
  directly or through stream interface.

****************************************************************************************************
 */
/*@{*/

/* Open socket.
 */
osalStream osal_socket_open(
    const os_char *parameters,
    void *option,
    osalStatus *status,
    os_int flags);

/* Close socket.
 */
void osal_socket_close(
    osalStream stream,
    os_int flags);

/* Accept connection from listening socket.
 */
osalStream osal_socket_accept(
    osalStream stream,
    os_char *remote_ip_addr,
    os_memsz remote_ip_addr_sz,
    osalStatus *status,
    os_int flags);

/* Flush written data to socket.
 */
osalStatus osal_socket_flush(
    osalStream stream,
    os_int flags);

/* Write data to socket.
 */
osalStatus osal_socket_write(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_memsz *n_written,
    os_int flags);

/* Read data from socket.
 */
osalStatus osal_socket_read(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_int flags);

/* Get socket parameter.
 */
os_long osal_socket_get_parameter(
    osalStream stream,
    osalStreamParameterIx parameter_ix);

/* Set socket parameter.
 */
void osal_socket_set_parameter(
    osalStream stream,
    osalStreamParameterIx parameter_ix,
    os_long value);

/* Wait for new data to read, time to write or operating system event, etc.
 */
#if OSAL_SOCKET_SELECT_SUPPORT
osalStatus osal_socket_select(
    osalStream *streams,
    os_int nstreams,
    osalEvent evnt,
    osalSelectData *selectdata,
    os_int timeout_ms,
    os_int flags);
#endif

/* Write packet (UDP) to stream.
 */
osalStatus osal_socket_send_packet(
    osalStream stream,
    const os_char *buf,
    os_memsz n,
    os_int flags);

/* Read packet (UDP) from stream.
 */
osalStatus osal_socket_receive_packet(
    osalStream stream,
    os_char *buf,
    os_memsz n,
    os_memsz *n_read,
    os_char *remote_addr,
    os_memsz remote_addr_sz,
    os_int flags);

/* Initialize OSAL sockets library.
 */
void osal_socket_initialize(
    osalNetworkInterface *nic,
    os_int n_nics,
    osalWifiNetwork *wifi,
    os_int n_wifi);

/* Shut down OSAL sockets library.
 */
void osal_socket_shutdown(void);

/* Are sockets initialized (most important with wifi, call always when opening the
   socket to maintain wifi state).
 */
osalStatus osal_are_sockets_initialized(
    void);

/* Keep the sockets library alive.
 */
#if OSAL_SOCKET_MAINTAIN_NEEDED
void osal_socket_maintain(void);
#else
#define osal_socket_maintain()
#endif

/*@}*/

#else

/* No socket support, define empty macros that we do not need to #ifdef code.
 */
#define osal_socket_initialize(n,c,w,d)
#define osal_socket_shutdown()
#define osal_socket_maintain()

/* No socket interface, allow build even if the define is used.
 */
#define OSAL_SOCKET_IFACE OS_NULL

#endif

#endif
