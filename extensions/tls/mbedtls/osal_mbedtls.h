/**

  @file    tls/common/osal_mbedtls.c
  @brief   OSAL stream API layer to use secure Mbed TLS sockets.
  @author  Pekka Lehtikoski
  @version 1.1
  @date    26.4.2021

  Secure network connectivity. Implementation of OSAL stream API and general network functionality
  using Mbed TLS.

  We do NOT want to use socket API trough mbedtls_net_* functions, but osal_stream_* api functions
  instead. This allows use of features implemented for socket stream transport wrapper.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_MBEDTLS_H_
#define OSAL_MBEDTLS_H_
#include "eosalx.h"
#if OSAL_TLS_SUPPORT==OSAL_TLS_MBED_WRAPPER

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "mbedtls/net_sockets.h"

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#include <stdlib.h>
#define mbedtls_time            time
#define mbedtls_time_t          time_t
#define mbedtls_fprintf         fprintf
#define mbedtls_printf          printf
#define mbedtls_exit            exit
#define MBEDTLS_EXIT_SUCCESS    EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE    EXIT_FAILURE
#endif /* MBEDTLS_PLATFORM_C */

/* Break the build if we do not have all components.
 */
#if !defined(MBEDTLS_ENTROPY_C) || \
    !defined(MBEDTLS_SSL_TLS_C) || !defined(MBEDTLS_SSL_CLI_C) || \
    !defined(MBEDTLS_NET_C) || !defined(MBEDTLS_CTR_DRBG_C)

    *** UNABLE TO BUILD ***
    MBEDTLS_ENTROPY_C and/or
    MBEDTLS_SSL_TLS_C and/or MBEDTLS_SSL_CLI_C and/or
    MBEDTLS_NET_C and/or MBEDTLS_CTR_DRBG_C and/or not defined.
#endif

#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#include <signal.h>

/** Mbed TLS specific global data related to TLS.
 */
typedef struct osalTLS
{
    /** Random number generator context.
     */
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;

    /** Certificate authority certificate
     */
    mbedtls_x509_crt cacert;

    /** Server only
     */
    mbedtls_x509_crt srvcert;
    mbedtls_pk_context pkey;
}
osalTLS;

#endif
#endif
