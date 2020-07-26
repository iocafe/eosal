/**

  @file    error/common/osal_error.h
  @brief   Error handling API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    1.2.2020

  This eosal error API is used by eosal, iocom, and pins libraries to report errors. It can
  also be used for application's error reporting. Or if application has it's own error
  reporting API, the error information from esal, iocom and pins can be mapped to it
  by setting an application specific error handler callback.

  ***** REPORTING ERRORS *****
  When an error occurs, the osal_error() function is called to record it. The code argument
  is error number. eosal status codes are enumerated in osalStatus. Other software modules
  have their own error enumerations. The module argument is module name string, like eosal_mod
  (constant string "eosal"). Module name together with code is unique definition of an
  error condition.

    void osal_error(
        osalErrorLevel level,
        const os_char *module,
        os_int code,
        const os_char *description);

  The function osal_clear_error() is provided as generic way to pass information to error
  handling that an error should be cleared. This actually just calls osal_error with
  error level OSAL_CLEAR_ERROR and NULL description.

    void osal_clear_error(
        const os_char *module,
        os_int code);

  ***** ERROR HANDLER *****
  The default error handler osal_default_error_handler() is useful only for first stages
  of testing. It just calls writes error messages to console or serial port, similarly to
  osal_debug. Custom error handler function needs to be implemented to have report
  errors in such way that is useful to the end user.

  Make a custom error handler like my_custom_error_handler() below:

    void my_custom_error_handler(
        osalErrorLevel level,
        const os_char *module,
        os_int code,
        const os_char *description,
        void *context)
    {
    }

  Set it as error handler (context can be any app specific pointer to pass to callback,
  here OS_NULL):

    osal_set_error_handler(my_custom_error_handler, OS_NULL);

  ***** WHY OSAL_ERROR AND OSAL_DEBUG_ERROR? *****
  Difference to osal_debug_error*(), osal_debug_assert(), osal_trace*() is the osal_error.h API
  is for run time error reporting in ready product, and osal_debug.h functions for software
  development only and are removed from compiled code by setting OSAL_DEBUG=0 or OSAL_TRACE=0.

  ***** IMPLEMENTATION HINTS *****
  Useful indication of microcontroller IO board errors in end user environment is a bit of
  challenge, we typically have no display, etc. Especially error reporting occurring during boot
  before network connection to server is established may be limited to a blinking LED.
  I have used coding like N short blinks followed by longer pause, and N being problem code.
  For example "1 = network unplugged", 2 = "no wifi networks", "3 = network unreachable",
  "4 = no reply from server", 5 = "no authorization".

  Errors occuring later is much simpler. Typically works well to indicate boolean signal
  for each error, indicating wether error is on or off. Plus counter how many times
  a specific error has occurred is sometimers useful.


  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_ERROR_H_
#define OSAL_ERROR_H_
#include "eosal.h"

/** Error level, how serious an error?
 */
typedef enum
{
    OSAL_INFO,
    OSAL_WARNING,
    OSAL_ERROR,
    OSAL_SYSTEM_ERROR,
    OSAL_CLEAR_ERROR
}
osalErrorLevel;

/* Module name used by eosal library to report errors.
 */
extern const os_char eosal_mod[];

/* Flags for osal_set_error_handler() function and in osalErrorHandler structure.
 */
#define OSAL_REPLACE_ERROR_HANDLER 0
#define OSAL_ADD_ERROR_HANDLER 1
#define OSAL_SYSTEM_ERROR_HANDLER 2
#define OSAL_APP_ERROR_HANDLER 0

/** Error handler function type.
 */
typedef void osal_error_handler(
    osalErrorLevel level,
    const os_char *module,
    os_int code,
    const os_char *description,
    void *context);

/* Structure to store error handle function pointer and context
 */
typedef struct
{
    /** Error handler function.
     */
    osal_error_handler *func;

    /** Error handler context.
     */
    osal_error_handler *context;

    /** Error handler flags, significant flag OSAL_SYSTEM_ERROR_HANDLER separates
        if this is system error handler (set by eosal/iocom) or application error handler.
        When error handler is set, application error handler can be replaced by
        application error handler and system error handler by system error handler.
     */
    os_short flags;
}
osalErrorHandler;

/* Report an error
 */
void osal_error(
    osalErrorLevel level,
    const os_char *module,
    os_int code,
    const os_char *description);

/* Report information message (error reporting API)
 */
void osal_info(
    const os_char *module,
    os_int code,
    const os_char *description);

/* Clear an error
 */
void osal_clear_error(
    const os_char *module,
    os_int code);

/* Set error handler (function to be called when error is reported).
 */
osalStatus osal_set_error_handler(
    osal_error_handler *func,
    void *context,
    os_short flags);

/* Default error handler function
 */
void osal_default_error_handler(
    osalErrorLevel level,
    const os_char *module,
    os_int code,
    const os_char *description,
    void *context);

#endif
