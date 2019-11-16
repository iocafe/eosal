/**

  @file    initialize/linux/osal_os_initialize.c
  @brief   Operating system specific OSAL initialization.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Operating system specific OSAL initialization and shut down.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#include <signal.h>
#include <sys/wait.h>

/**
****************************************************************************************************
  Signal handlers.
****************************************************************************************************
*/
static void osal_linux_sighup(
    int signum)
{
    osal_debug_error("SIGHUP");
}

static void osal_linux_sigfpe(
    int signum)
{
    osal_debug_error("SIGFPE");
}

static void osal_linux_sigalrm(
    int signum)
{
    osal_debug_error("SIGALRM");
}

static void osal_linux_sigchld(
    int signum)
{
    int x;
    osal_debug_error("SIGCHLD");
    waitpid(-1, &x, WNOHANG);
}

static void osal_linux_terminate_by_signal(
    int signum)
{
    osal_global->exit_process = OS_TRUE;
}


/**
****************************************************************************************************

  @brief Operating system specific OSAL library initialization.
  @anchor osal_init_os_specific

  The osal_init_os_specific() function does operating system specific initialization
  OSAL library for use.

  @param  flags Bit fields. OSAL_INIT_DEFAULT (0) for normal initalization.
          OSAL_INIT_NO_LINUX_SIGNAL_INIT not to initialize linux signals.

  @return  None.

****************************************************************************************************
*/
void osal_init_os_specific(
    os_int flags)
{
    if ((flags & OSAL_INIT_NO_LINUX_SIGNAL_INIT) == 0)
    {
        /* Ignore broken sockets, closing controlling terminal, closed child process
         */
        signal (SIGPIPE, SIG_IGN);

        /* Ignore, but call oedebug_error() leave note if controlling terminal process is closed,
         */
        signal (SIGHUP, osal_linux_sighup);
        signal (SIGFPE, osal_linux_sigfpe);
        signal (SIGALRM, osal_linux_sigalrm);

        /* Handle closed child process.
         */
        signal (SIGCHLD, osal_linux_sigchld);

        /* Terminate process on following signals.
         */
        signal (SIGTERM, osal_linux_terminate_by_signal);
        signal (SIGQUIT, osal_linux_terminate_by_signal);
        signal (SIGINT, osal_linux_terminate_by_signal);
        signal (SIGTSTP, osal_linux_terminate_by_signal);
        signal (SIGABRT, osal_linux_terminate_by_signal);
    }
}


/**
****************************************************************************************************

  @brief Shut down operating system specific part of the OSAL library.
  @anchor osal_shutdown

  The osal_shutdown_os_specific() function...

  @return  None.

****************************************************************************************************
*/
void osal_shutdown_os_specific(
    void)
{
}


