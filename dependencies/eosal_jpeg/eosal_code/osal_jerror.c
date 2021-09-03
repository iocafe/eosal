/**

  @file    eosal_jpeg/eosal_code/osal_jerror.c
  @brief   eosal API for libjpeg.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    5.5.2020

  This file contains simple error-reporting and trace-message routines. These routines are
  used by both the compression and decompression code.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

  Copyright (C) 1991-1998, Thomas G. Lane. This file is derived from work of the Independent
  JPEG Group's software.

****************************************************************************************************
*/
#include "eosal_jpeg.h"
#if IOC_USE_JPEG_COMPRESSION
#include "code/jversion.h"
#include "code/jerror.h"

/* Create the message string table. We do this from the master message list in jerror.h by
   re-reading  * jerror.h with a suitable definition for macro JMESSAGE. The message table
   is made an external symbol just in case any applications want to refer to it directly.
 */
#ifdef NEED_SHORT_EXTERNAL_NAMES
#define jpeg_std_message_table	eosal_jMsgTable
#endif

#define JMESSAGE(code,string)	string ,

const char * const jpeg_std_message_table[] = {
#include "code/jerror.h"
  NULL
};



/**
****************************************************************************************************

  @brief Actual output of an error or trace message.

  This sends JPEG messages to eosal debug error output.

  @param  cinfo JPEG library's internal state structure pointer?
  @return None.

****************************************************************************************************
*/
static void output_message(
    j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message) (cinfo, buffer);
    osal_debug_error(buffer);
}


/**
****************************************************************************************************

  @brief Decide whether to emit a trace or warning message.

  msg_level is one of:
    -1: recoverable corrupt-data warning, may want to abort.
     0: important advisory messages (always display to user).
     1: first level of tracing detail.
     2,3,...: successively more detailed tracing messages.
  An application might override this method if it wanted to abort on warnings
  or change the policy about which messages to display.
  This sends JPEG messages to eosal debug error output.

  @param  cinfo JPEG library's internal state structure pointer?
  @return None.

****************************************************************************************************
*/
static void emit_message(
    j_common_ptr cinfo,
    int msg_level)
{
    struct jpeg_error_mgr * err = cinfo->err;

    if (msg_level < 0)
    {
        /* It's a warning message.  Since corrupt files may generate many warnings,
         * the policy implemented here is to show only the first warning,
         * unless trace_level >= 3.
         */
        if (err->num_warnings == 0 || err->trace_level >= 3) {
            (*err->output_message) (cinfo);
        }

        /* Always count warnings in num_warnings. */
        err->num_warnings++;
    }
    else {
        /* It's a trace message.  Show it if trace_level >= msg_level. */
        if (err->trace_level >= msg_level) {
            (*err->output_message) (cinfo);
        }
    }
}


/**
****************************************************************************************************

  @brief Format JPEG library message.

  Format a message string for the most recent JPEG error or message. The message is stored into
  buffer, which should be at least JMSG_LENGTH_MAX characters. Note that no '\n' character is
  added to the string. Few applications should need to override this method.

  @param  cinfo JPEG library's internal state structure pointer?
  @return None.

****************************************************************************************************
*/
static void format_message(
    j_common_ptr cinfo,
    char * buffer)
{
    struct jpeg_error_mgr * err = cinfo->err;
    int msg_code = err->msg_code;
    const char * msgtext = NULL;
    const char * msgptr;
    char ch;
    boolean isstring;

    /* Look up message string in proper table */
    if (msg_code > 0 && msg_code <= err->last_jpeg_message) {
        msgtext = err->jpeg_message_table[msg_code];
    }
    else if (err->addon_message_table != NULL &&
         msg_code >= err->first_addon_message &&
         msg_code <= err->last_addon_message)
    {
        msgtext = err->addon_message_table[msg_code - err->first_addon_message];
    }

    /* Defend against bogus message number */
    if (msgtext == NULL) {
        err->msg_parm.i[0] = msg_code;
        msgtext = err->jpeg_message_table[0];
    }

    /* Check for string parameter, as indicated by %s in the message text */
    isstring = FALSE;
    msgptr = msgtext;
    while ((ch = *msgptr++) != '\0') {
        if (ch == '%') {
          if (*msgptr == 's') isstring = TRUE;
          break;
        }
    }

    os_strncpy(buffer, msgtext, JMSG_LENGTH_MAX);
    if (isstring) {
        /* sprintf(buffer, msgtext, err->msg_parm.s); */
        os_strncat(buffer, ": ", JMSG_LENGTH_MAX);
        os_strncat(buffer, err->msg_parm.s, JMSG_LENGTH_MAX);
    }
    /* else {
    sprintf(buffer, msgtext,
        err->msg_parm.i[0], err->msg_parm.i[1],
        err->msg_parm.i[2], err->msg_parm.i[3],
        err->msg_parm.i[4], err->msg_parm.i[5],
        err->msg_parm.i[6], err->msg_parm.i[7
    osal_debug_error(msgtext);
    } */
}


/**
****************************************************************************************************

  @brief Reset error state variables at start of a new image.

  This is called during compression startup to reset trace/error processing to default state,
  without losing any application-specific method pointers. An application might possibly want
  to override this method if it has additional error processing state.

  @param  cinfo JPEG library's internal state structure pointer?
  @return None.

****************************************************************************************************
*/
static void reset_error_mgr (
    j_common_ptr cinfo)
{
    cinfo->err->num_warnings = 0;
    cinfo->err->msg_code = 0;
}


/**
****************************************************************************************************

  @brief Fill in the standard error-handling methods in a jpeg_error_mgr object.

 Typical call is:
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr err;
    cinfo.err = jpeg_std_error(&err);

  after which the application may override some of the methods.

  @param  cinfo JPEG library's internal state structure pointer?
  @return None.

****************************************************************************************************
*/
GLOBAL(struct jpeg_error_mgr *)
jpeg_std_error (struct jpeg_error_mgr * err)
{
    err->emit_message = emit_message;
    err->output_message = output_message;
    err->format_message = format_message;
    err->reset_error_mgr = reset_error_mgr;

    err->trace_level = 0;		/* default = no tracing */
    err->num_warnings = 0;      /* no warnings emitted yet */
    err->msg_code = 0;          /* may be useful as a flag for "no error" */

    /* Initialize message table pointers */
    err->jpeg_message_table = jpeg_std_message_table;
    err->last_jpeg_message = (int) JMSG_LASTMSGCODE - 1;

    err->addon_message_table = NULL;
    err->first_addon_message = 0;	/* for safety */
    err->last_addon_message = 0;

    return err;
}

#endif
