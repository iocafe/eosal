/**

  @file    initialize/linux/osal_os_initialize.c
  @brief   Operating system specific OSAL initialization.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Operating system specific OSAL initialization and shut down.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#ifdef OSAL_LINUX
#include <signal.h>
#include <sys/wait.h>

/**
****************************************************************************************************
  Signal handlers.
****************************************************************************************************
*/
typedef void osal_signal_handler_func(
    int signum);

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
    osal_request_exit();
}


/**
****************************************************************************************************

  @brief Set signal handler callback function.
  @anchor osal_set_signal

  The osal_set_signal() function sets signal handler function or if func is SIG_IGN, marks
  signal to be ignored. The function is used to replace depreceated unix signal() function,
  and does the same thing.

  @param   sig Signal for which handler is set, for example SIGPIPE to handle socket and pipe
           signals.
  @param   func Pointer to signal handler function or SIG_IGN to ignore the signal.
  @return  None.

****************************************************************************************************
*/
static void osal_set_signal(
    os_int sig,
    osal_signal_handler_func *func)
{
    struct sigaction new_actn;

    os_memclear(&new_actn, sizeof(new_actn));
    new_actn.sa_handler = func;
    sigemptyset (&new_actn.sa_mask);
    new_actn.sa_flags = 0;
    sigaction (sig, &new_actn, NULL);
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
        osal_set_signal(SIGPIPE, SIG_IGN);

        /* Ignore, but call oedebug_error() leave note if controlling terminal process is closed,
         */
        osal_set_signal(SIGHUP, osal_linux_sighup);
        osal_set_signal(SIGFPE, osal_linux_sigfpe);
        osal_set_signal(SIGALRM, osal_linux_sigalrm);

        /* Handle closed child process.
         */
        osal_set_signal(SIGCHLD, osal_linux_sigchld);

        /* Terminate process on following signals.
         */
        osal_set_signal(SIGTERM, osal_linux_terminate_by_signal);
        osal_set_signal(SIGQUIT, osal_linux_terminate_by_signal);
        osal_set_signal(SIGINT, osal_linux_terminate_by_signal);
        osal_set_signal(SIGTSTP, osal_linux_terminate_by_signal);
        osal_set_signal(SIGABRT, osal_linux_terminate_by_signal);
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


/**
****************************************************************************************************

  @brief Reboot the computer.
  @anchor osal_reboot

  The osal_reboot() function...

  @param   flags Reserved for future, set 0 for now.
  @return  None.

****************************************************************************************************
*/
void osal_reboot(
    os_int flags)
{
}

#endif
