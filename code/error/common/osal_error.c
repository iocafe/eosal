/**

  @file    error/common/osal_error.c
  @brief   Error handling API.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    1.2.2020

  This eosal error API is used by eosal, iocom, and pins libraries to report errors.
  See osal_error.h for description.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"

/** Module name used by eosal library to report errors.
 */
const os_char eosal_mod[] = "eosal";


/**
****************************************************************************************************

  @brief Report an error
  @anchor osal_error

  When an error occurs, the osal_error() function is called to record it. The code argument
  is error number. eosal status codes are enumerated in osalStatus. Other software modules
  have their own error enumerations. The module argument is module name string, like eosal_mod
  (constant string "eosal"). Module name together with code is unique definition of an
  error condition.

  @param   level Seriousness or error or clear error request. One of: OSAL_INFO, OSAL_WARNING,
           OSAL_ERROR, OSAL_SYSTEM_ERROR, or OSAL_CLEAR_ERROR.
  @param   Module which reported the error, string. Typically static constant, like eosal_mod
           for the eosal library.
  @param   code Status code or error number, like osalStatus enumeration for the eosal library.
  @param   description Text description of an error or additional information . Optional,
           can be OS_NULL.

  @return  None.

****************************************************************************************************
*/
void osal_error(
    osalErrorLevel level,
    const os_char *module,
    os_int code,
    const os_char *description)
{
    osalErrorHandler *error_handler;
    int i;
    os_boolean app_error_handler_called = OS_FALSE;

    /* Call error handlers and remember if we an application error handler.
     */
    for (i = 0; i < OSAL_MAX_ERROR_HANDLERS; i++)
    {
        error_handler = &osal_global->error_handler[i];
        if (error_handler->func != OS_NULL)
        {
            error_handler->func(level, module, code,
                description, error_handler->context);

            if ((error_handler->flags & OSAL_SYSTEM_ERROR_HANDLER) == 0) {
                app_error_handler_called = OS_TRUE;
            }
        }
    }

    /* If no application error handler called, call default error handler.
     */
    if (!app_error_handler_called)
    {
        osal_default_error_handler(level, module, code, description, OS_NULL);
    }
}


/**
****************************************************************************************************

  @brief Report information message (error reporting API)
  @anchor osal_info

  The osal_info() function exists for readability. It is more understandable to call osal_info()
  to pass information messages to error handling than to call osal_error() with OSAL_INFO flag.

  @param   Module which reported the error, string. Typically static constant, like eosal_mod
           for the eosal library.
  @param   code Status code or error number, like osalStatus enumeration for the eosal library.

  @return  None.

****************************************************************************************************
*/
void osal_info(
    const os_char *module,
    os_int code,
    const os_char *description)
{
    osal_error(OSAL_INFO, module, code, description);
}


/**
****************************************************************************************************

  @brief Clear an error (error reporting API)
  @anchor osal_clear_error

  The osal_clear_error() function provides as generic way to pass information to error
  handling that an error should be cleared. This actually just calls osal_error with
  error level OSAL_CLEAR_ERROR and NULL description. There is not separate callback for
  clearing error, the same error handler callback functions gets called with OSAL_CLEAR_ERROR
  argument.

  @param   Module which reported the error, string. Typically static constant, like eosal_mod
           for the eosal library.
  @param   code Status code or error number, like osalStatus enumeration for the eosal library.
  @param   description Text description of an error or additional information . Optional,
           can be OS_NULL.

  @return  None.

****************************************************************************************************
*/
void osal_clear_error(
    const os_char *module,
    os_int code)
{
    osal_error(OSAL_CLEAR_ERROR, module, code, OS_NULL);
}


/**
****************************************************************************************************

  @brief Set error handler (function to be called when error is reported).
  @anchor osal_set_error_handler

  The osal_set_error_handler() function saves pointer to custom error handler function and
  application context pointer. After this, the custom error handler function is called
  instead of default error handler.

  Typically eosal and iocom set system error handler for example to maintain network status
  based on osal_error() calls, etc. Outside these, application can set own error handler.
  If application error handler is set, default error handler is not called.

  SETTING ERROR HANDLERS IS NOT MULTITHREAD SAFE AND THUS ERROR HANDLER FUNCTIONS NEED BE
  SET BEFORE THREADS ARE COMMUNICATION, ETC THREADS WHICH CAN REPORT ERRORS ARE CREATED.

  @param   func Pointer to application provided error handler function. Set OS_NULL to
           remove error handler.
  @param   context Application specific pointer, to be passed to error handler when it is
           called.
  @param   flags Bits: OSAL_REPLACE_ERROR_HANDLER (0) or OSAL_ADD_ERROR_HANDLER (1),
           OSAL_APP_ERROR_HANDLER (0) or OSAL_SYSTEM_ERROR_HANDLER (2).

  @return  If successfull, the function returns OSAL_SUCCESS. If there are already
           maximum number of error handlers (OSAL_MAX_ERROR_HANDLERS), the function
           returns OSAL_STATUS_FAILED.

****************************************************************************************************
*/
osalStatus osal_set_error_handler(
    osal_error_handler *func,
    void *context,
    os_short flags)
{
    int i;
    osalErrorHandler *error_handler;

    /* If we are replacing existing error handler, remove old ones. If we are
       replacing application error handler, remove application error handlers.
       If we are replacing system error handler, remove system error handlers.
     */
    if ((flags & OSAL_ADD_ERROR_HANDLER) == 0)
    {
        for (i = 0; i < OSAL_MAX_ERROR_HANDLERS; i++)
        {
            error_handler = &osal_global->error_handler[i];
            if (error_handler->func &&
                (error_handler->flags & OSAL_SYSTEM_ERROR_HANDLER)
                == (flags & OSAL_SYSTEM_ERROR_HANDLER))
            {
                error_handler->func = OS_NULL;
            }
        }
    }

    /* Add error handler.
     */
    for (i = 0; i < OSAL_MAX_ERROR_HANDLERS; i++)
    {
        error_handler = &osal_global->error_handler[i];
        if (error_handler->func == OS_NULL)
        {
            error_handler->func = func;
            error_handler->context = context;
            error_handler->flags = flags;
            return OSAL_SUCCESS;
        }
    }

    /* Too many error handlers.
     */
    return OSAL_STATUS_FAILED;
}


/**
****************************************************************************************************

  @brief Default error handler function
  @anchor osal_default_error_handler

  The osal_default_error_handler() writes error message to debug output (console, serial port,
  etc). This function is useful only for first stages of testing. Custom error handler function
  needs to be implemented to have report errors in such way that is useful to the end user.

  The osal_default_error_handler can be considered as application error handler, since it
  is not called if application error handler is set.

  @param   level Seriousness or error or clear error request. One of: OSAL_INFO, OSAL_WARNING,
           OSAL_ERROR, OSAL_SYSTEM_ERROR, or OSAL_CLEAR_ERROR.
  @param   Module which reported the error, string. Typically static constant, like eosal_mod
           for the eosal library.
  @param   code Status code or error number, like osalStatus enumeration for the eosal library.
  @param   description Text description of an error or additional information . Optional,
           can be OS_NULL.
  @param   context Application specific pointer, not used by default error handler.

  @return  None.

****************************************************************************************************
*/
void osal_default_error_handler(
    osalErrorLevel level,
    const os_char *module,
    os_int code,
    const os_char *description,
    void *context)
{
    os_char line[128], nbuf[OSAL_NBUF_SZ];
    const os_char *level_text;

    /* Select text describing how serious this error is. Ignore clear error requests.
     */
    switch (level) {
        case OSAL_INFO: level_text = "info"; break;
        case OSAL_WARNING: level_text = "warning"; break;
        case OSAL_ERROR: level_text = "error"; break;
        case OSAL_SYSTEM_ERROR: level_text = "system"; break;
        default: level_text = "?"; break;
        case OSAL_CLEAR_ERROR: return;
    }

    /* Format error message.
     */
    os_strncpy(line, module, sizeof(line));
    os_strncat(line, "#", sizeof(line));
    osal_int_to_str(nbuf, sizeof(nbuf), code);
    os_strncat(line, nbuf, sizeof(line));
    os_strncat(line, " ", sizeof(line));
    os_strncat(line, level_text, sizeof(line));
    os_strncat(line, ": ", sizeof(line));
    os_strncat(line, description, sizeof(line));
    os_strncat(line, "\n", sizeof(line));

    /* Write the error message on debug console, if any.
     */
    osal_console_write(line);
}