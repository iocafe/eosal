/**

  @file    defs/common/osal_global.h
  @brief   Global OSAL state structure.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  This header file contains definition of OSAL global state structure.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_GLOBAL_H_
#define OSAL_GLOBAL_H_
#include "eosal.h"

/* We need OSAL_AES_KEY_SZ
 */
#if OSAL_AES_CRYPTO_SUPPORT
#include "extensions/tls/common/osal_aes_crypt.h"
#endif

struct osalMutexStruct;
struct osalNetworkState;
struct osalSocketGlobal;

/* Extension module shutdown function type.
 */
typedef void osal_shutdown_func(void);

#if OSAL_TLS_SUPPORT==OSAL_TLS_MBED_WRAPPER
    struct osalTLS;
#endif

/** Secret (random number used as security basis) size as binary, 256 bits = 32 bytes.
 */
#define OSAL_SECRET_BIN_SZ 32

/** Size of string buffer for storing the secret or password.
 */
#define OSAL_SECRET_STR_SZ 46

/** Size of unique ID (96 bits, big enough that we can reasonably assume not to get two same 
    ones in same network, but small enough to avoid increasing message size).
 */
#define OSAL_UNIQUE_ID_BIN_SZ 12

/* Structure for storing secret and unique id.
 */
typedef struct osalSecretStorage
{
    /** Secret as string.
     */
    os_char secret_bin[OSAL_SECRET_BIN_SZ];

    /** Unique ID of the device.
     */
    os_char unique_id_bin[OSAL_UNIQUE_ID_BIN_SZ];
}
osalSecretStorage;

/** Sizeof nickname buffer.
 */
#define OSAL_NICKNAME_SZ 16

/* Some common strings.
 */
extern OS_CONST_H os_char osal_str_asterisk[];
extern OS_CONST_H os_char osal_str_empty[];

/**
****************************************************************************************************

  @name Global Structure
  @anchor osalGlobalStruct

  The global structure saves OSAL library state. In practise this means all internal variables
  of the OSAL library. The variables are stored within a structure and the structure is always
  referred through osal_global pointer. Thus DLLs can share the same global structure with the
  process which loaded the DLL. This will require only setting osal_global pointer in DLL.

****************************************************************************************************
*/
typedef struct
{
    /** OSAL library initialized flag.
     */
    os_boolean osal_initialized;

    /** Set to OS_TRUE to request "terminate the process".
     */
    os_boolean exit_process;

    /** Quiet mode silences debug prints to allow user to operate console.
     */
    os_boolean quiet_mode;

#if OSAL_MULTITHREAD_SUPPORT
    /** System mutex. System mutex is used to synchronize access to global variables, etc.
     */
    struct osalMutexStruct *system_mutex;

#if OSAL_TIME_CRITICAL_SYSTEM_LOCK
    /** System mutex lock count. This is incremented by os_lock() function
        and decremented by os_unlock() function.
     */
    os_int system_mutex_lock_count;

    /** Saved priority when system mutex was locked. This is set by os_lock()
        function and restored by os_unlock() function. The value is
        operating system's priority, not OSAL priority.
     */
    os_int system_mutex_enter_priority;

    /** Saved system mutex thread handle. This is saved by os_lock()
        function and used by os_unlock() function.
     */
    void *system_mutex_thread;
#endif

#if OSAL_OS_EVENT_LIST_SUPPORT
    /** List of operating systems to set at exit.
     */
    osalEventList atexit_events_list;
#endif

    /** Number of threads created by osal_thread_create().
     */
    os_short thread_count;

#endif

    /* Memory allocation related, see osal_memory.c
     */
#if OSAL_MEMORY_MANAGER

    /** Pointer to function to allocate memory block from the operating system.
     */
    osal_sysmem_alloc_func *sysmem_alloc_func;

    /** Pointer to function to release a block of memory back to operating system.
     */
    osal_sysmem_free_func *sysmem_free_func;

    /** Memory manager state structure, see osal_memory.h.
     */
    osalMemManagerState memstate;
#endif

    /* Security "secret"
     */
#if OSAL_SECRET_SUPPORT
    /** Secret initialized flag.
     */
    os_boolean secret_initialized;

    /** Secret and unique ID as binary.
     */
    osalSecretStorage saved;

    /** Secret as string. This is used for encrypting private key of TLS server
        so it can be saved as normal data, etc.
     */
    os_char secret_str[OSAL_SECRET_STR_SZ];

    /** Automatically generated IO node password.
     */
    os_char auto_password[OSAL_SECRET_STR_SZ];

#if OSAL_AES_CRYPTO_SUPPORT
    /** Key for encrypting secret and private server key for optional extra security.
        Used to protect the keys on microcontroller against attacker using JTAG
        debugger, or if PC computer security is broken.
     */
    os_uchar secret_crypt_key[OSAL_AES_KEY_SZ];
#endif

#endif

#if OSAL_NICKNAME_SUPPORT
    /** Nickname.
     */
    os_char nickname[OSAL_NICKNAME_SZ];
#endif

    /* Resource monitor related, see osal_resource_monitor.c.
     */
#if OSAL_RESOURCE_MONITOR

    /** Resource monitor state structure, see osal_resource_monitor.h.
     */
    osalResourceMonitorState resstate;
#endif

#if OSAL_TLS_SUPPORT
    struct osalTLS *tls;
#endif

    /** System timer parameter. Values differ for each operating system.
     */
    os_int64 sys_timer_param;

#if OSAL_SOCKET_SUPPORT
    /** Pointer to global socket structure
     */
    struct osalSocketGlobal *socket_global;

    /** Shut down function to close sockets or TLS library. Set if sockets/TLS
        library is initialized and needs shut down code. osal_shutdown() function
        calls this.
     */
    osal_shutdown_func *sockets_shutdown_func;
#endif

#if OSAL_PROCESS_CLEANUP_SUPPORT
  #if OSAL_SERIAL_SUPPORT
    /** Shut down function to close library. Set if serial communication library
        is initialized. osal_shutdown() function calls this.
     */
    osal_shutdown_func *serial_shutdown_func;
  #endif

  #if OSAL_BLUETOOTH_SUPPORT
    osal_shutdown_func *bluetooth_shutdown_func;
  #endif
#endif

#if OSAL_INTERRUPT_LIST_SUPPORT
    /* Linked list of interrupt control function pointers.
     */
    struct osalInterruptInfo *first_listed_interrupt;
    struct osalInterruptInfo *last_listed_interrupt;
    os_short interrupts_disable_count;
#endif

#if OSAL_MAX_ERROR_HANDLERS > 0
   /** Error handler function and context pointers.
     */
    osalNetEventHandler event_handler[OSAL_MAX_ERROR_HANDLERS];
#endif

    /** Network state structure pointer (osal_net_state.c).
     */
    struct osalNetworkState *net_state;
}
osalGlobalStruct;

#define osal_go() (!osal_global->exit_process)
#define osal_stop() (osal_global->exit_process)

#if OSAL_PROCESS_CLEANUP_SUPPORT && OSAL_MULTITHREAD_SUPPORT
    void osal_request_exit(void);
    void osal_wait_for_threads_to_exit(void);
#else
    #define osal_request_exit()
    #define osal_wait_for_threads_to_exit();
#endif


/* Quiet mode silences debug prints, etc. to allow user to operate console.
 */
os_boolean osal_quiet(
    os_boolean enable);

/* Get global nickname for the device (~ process).
 */
#if OSAL_NICKNAME_SUPPORT
    #define osal_nickname() (osal_global->nickname)
    #define osal_set_nickname(n) os_strncpy(osal_global->nickname, (n), OSAL_NICKNAME_SZ)
#else
    #define osal_nickname() (OS_NULL)
    #define osal_set_nickname(n)
#endif

#endif
