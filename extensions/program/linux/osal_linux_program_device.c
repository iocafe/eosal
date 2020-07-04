/**

  @file    eosal/extensions/program/linux/osal_linux_program_device.c
  @brief   Write IO device program to executables.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    1.7.2020

  Linux installation packages are .deb files. When a program in linux based device receives
  a .deb file trough IOCOM, it quietly installs it. The installation is done as root user.

  File permissions, owner and group. All files in installation package are owned by root.
  If binary needs to update software, set setuid bit for it needs to be set so it will
  run as root. All files which are not to be modified by user should have 0755 permissions,
  except 04755 for binary files capable of software updates. Data files which can be modified
  by user should have 0664 or 0666. Using 0664 allows extra protection by user group, but
  this requires some extra configuration.

  The setuid attribute bit for binary files: To run application You don't need to do anything on
  the C side. Just change the binary to be owned by the user you want to use (here root),
  enable the setuid bit in the binary (chmod u+s), and you're all set. No need to call
  setuid(0) from C

  See "200702-linux-installation-packages.rst" in documentation.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_DEVICE_PROGRAMMING_SUPPORT

static os_char deb_path[] = "/tmp/iocomtempprog.deb";

/* Installer state structure stores what the installer is "doing now".
 */
typedef struct
{
    osalStream deb_stream;
    os_boolean installer_thread_running;
}
osalInstallerState;

static osalInstallerState osal_istate;


/* Forward referred static functions.
 */
static void osal_close_tmp_file(void);
static void osal_delete_tmp_file(void);
static osalStatus osal_install_package(void);


/**
****************************************************************************************************

  @brief Clear installation state.
  @anchor osal_initialize_programming

  The osal_initialize_programming() function just clears instalalation state. This function is
  called after boot to ensure that installation state is zeroes.

  @return  None.

****************************************************************************************************
*/
void osal_initialize_programming(void)
{
    os_memclear(&osal_istate, sizeof(osal_istate));
}


/**
****************************************************************************************************

  @brief Start the device programming.
  @anchor osal_start_device_programming

  The osal_start_device_programming() function is called when program transfer starts. It checks
  if device is ready for programming and opens temporary file into which the debian installation
  package will be written.

  @return  OSAL_SUCCESS if the the installation can be started.

****************************************************************************************************
*/
osalStatus osal_start_device_programming(void)
{
    osalStatus s;

    /*  If installer is already running.
     */
    if (osal_istate.installer_thread_running) {
        osal_debug_error("starting installation failed, installer already running");
        return OSAL_STATUS_FAILED;
    }

    osal_trace2("start programming");
    osal_close_tmp_file();

    osal_istate.deb_stream = osal_file_open(deb_path, OS_NULL, &s, OSAL_STREAM_WRITE);
    if (osal_istate.deb_stream == OS_NULL) return s;
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Append data to debian installation package.
  @anchor osal_program_device

  The osal_program_device() function is called when data is received from iocom to append it
  to temporary file to hold the debian package.

  @return  OSAL_SUCCESS if all is fine. Other values indicate an error.

****************************************************************************************************
*/
osalStatus osal_program_device(
    os_char *buf,
    os_memsz buf_sz)
{
    os_memsz n_written;
    osalStatus s;

    osal_trace2_int("programming device, bytes=", buf_sz);
    if (osal_istate.deb_stream == OS_NULL) {
        return OSAL_STATUS_FAILED;
    }
    s = osal_file_write(osal_istate.deb_stream, buf, buf_sz, &n_written, OSAL_STREAM_DEFAULT);
    if (s || n_written != buf_sz) {
        osal_cancel_device_programming();
        return s;
    }

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Install succesfully transferred debian package.
  @anchor osal_finish_device_programming

  The osal_finish_device_programming() function is called when all data in debian package has
  been transferred. The function closes the temporary deian file, installs the debian package
  and deletes the temporary file.

  @return  OSAL_SUCCESS if all is fine. Other values indicate an error.

****************************************************************************************************
*/
osalStatus osal_finish_device_programming(
    os_uint checksum)
{
    OSAL_UNUSED(checksum);

    osal_trace2("finish programming");
    if (osal_istate.deb_stream == OS_NULL) {
        return OSAL_STATUS_FAILED;
    }
    osal_close_tmp_file();
    osal_install_package();

    /* osal_reboot(0); */

    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Cancel package installation.
  @anchor osal_cancel_device_programming

  The osal_cancel_device_programming() function can be called when for example part of debian
  installation package has been transferred, but the socket connection breaks. The function
  closes and deletes the temporary file (if any).
  @return  None

****************************************************************************************************
*/
void osal_cancel_device_programming(void)
{
    osal_trace2("cancel programming");
    osal_close_tmp_file();
    osal_delete_tmp_file();
}


/**
****************************************************************************************************

  @brief Close the temporary file.
  @anchor osal_close_tmp_file

  The osal_close_tmp_file() function...
  @return  None

****************************************************************************************************
*/
static void osal_close_tmp_file(void)
{
    if (osal_istate.deb_stream)
    {
        osal_file_close(osal_istate.deb_stream, OSAL_STREAM_DEFAULT);
        osal_istate.deb_stream = OS_NULL;
    }
}


/**
****************************************************************************************************

  @brief Delete the temporary file.
  @anchor osal_delete_tmp_file

  The osal_delete_tmp_file() function...
  @return  None

****************************************************************************************************
*/
static void osal_delete_tmp_file(void)
{
    osal_remove(deb_path, 0);
}


#if OSAL_MULTITHREAD_SUPPORT
/**
****************************************************************************************************

  @brief Thread fuction to run the installation.
  @anchor osal_installer_thread

  The osal_installer_thread() function call "dpkg" to to install the debian package, and once
  ready restarts the application.

  dpkg -i --force-all iocomtempprog.deb

  @param   prm Pointer to parameters for new thread, pointer to end point object.
  @param   done Event to set when parameters have been copied to entry point
           functions own memory.

  @return  None.

****************************************************************************************************
*/
static void osal_installer_thread(
    void *prm,
    osalEvent done)
{
    osalStatus s;
    static os_char *const argv[] = {"dpkg", "-i", "--force-all", deb_path, OS_NULL};
    OSAL_UNUSED(prm);

    osal_trace("program device: installer thread created");

    /* Set "installer running" flag and let the thread which created this one proceed.
     */
    osal_istate.installer_thread_running = OS_TRUE;
    osal_event_set(done);

    s = osal_create_process("dpkg", argv, OSAL_PROCESS_WAIT|OSAL_PROCESS_ELEVATE);
    if (s) {
        osal_debug_error("debian package installation failed");
    }

    osal_delete_tmp_file();
    osal_istate.installer_thread_running = OS_FALSE;
}
#endif


/**
****************************************************************************************************

  @brief Start thread which installs the debian package.
  @anchor osal_install_package

  The osal_install_package() function starts the thread to do the installation. Installation is
  run in it's own thread for twp reasons:
  1. Program can continue operating normally while installing.
  2. Creating elevated process with root priviliges does modify threads real user and group.
     When done in own thread, this will not effect rest of the application.

  @return  OSAL_SUCCESS if the function completed successfully, other return values indicate an
           error.

****************************************************************************************************
*/
static osalStatus osal_install_package(void)
{
    osalThreadOptParams opt;
    os_memclear(&opt, sizeof(osalThreadOptParams));
    opt.thread_name = "installer";

    if (osal_istate.installer_thread_running) {
        return OSAL_STATUS_FAILED;
    }

    osal_thread_create(osal_installer_thread, OS_NULL,
        OS_NULL, OSAL_THREAD_DETACHED);

    return OSAL_SUCCESS;
}

#endif
