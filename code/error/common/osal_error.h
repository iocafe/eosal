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
  When an error occurs, the osal_error() function is called to recors it. The status argument
  is error number. eosal status codes are enumerated in osalStatus. Other software modules
  have their own error enumerations. The module argument is module name string, like eosal_mod
  (constant string "eosal"). Module name together with status is unique definition of an
  error state.

    void osal_error(
        osalErrorLevel level,
        const os_char *module,
        os_int status,
        const os_char *description);

  The function osal_clear_error() is provided as generic way to pass information to error
  handling that an error should be cleared. This actually just calls osal_error with
  error level OSAL_CLEAR_ERROR and NULL description.

    void osal_clear_error(
        const os_char *module,
        os_int status);

  ***** ERROR HANDLER *****
  The default error handler osal_default_error_handler() is useful only for first stages
  of testing. It just calls writes error messages to console or serial port, similarly to
  osal_debug. Custom error handler function needs to be implemented to have report
  errors in such way that is useful to the end user.

  Make a custom error handler like my_custom_error_handler() below:

    void my_custom_error_handler(
        osalErrorLevel level,
        const os_char *module,
        os_int status,
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

/** Error level, how serious an error?
 */
typedef enum
{
    OSAL_INFO,
    OSAL_WARNING,
    OSAL_ERROR,
    OSAL_CLEAR_ERROR
}
osalErrorLevel;

/* Module name used by eosal library to report errors.
 */
const extern os_char eosal_mod;


/* Report an error
 */
void osal_error(
    osalErrorLevel level,
    const os_char *module,
    os_int status,
    const os_char *description);

/* Clear an error
 */
void osal_clear_error(
    const os_char *module,
    os_int status);

/* Error handler function type.
 */
typedef void osal_error_handler(
    osalErrorLevel level,
    const os_char *module,
    os_int status,
    const os_char *description,
    void *context);

/* Set error handler (function to be called when error is reported).
 */
void osal_set_error_handler(
    osal_error_handler *func,
    void *context);

/* Default error handler function
 */
void osal_default_error_handler(
    osalErrorLevel level,
    const os_char *module,
    os_int status,
    const os_char *description,
    void *context);
