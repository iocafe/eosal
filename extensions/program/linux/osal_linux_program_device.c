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
static osalStream deb_stream = OS_NULL;

/* Forward referred static functions.
 */
static void osal_close_tmp_file(void);
static void osal_delete_tmp_file(void);
static void osal_install_package(void);


void osal_initialize_programming(void)
{
    deb_stream = OS_NULL;

osal_install_package();

}


osalStatus osal_start_device_programming(void)
{
    osalStatus s;

osal_debug_error("HERE start prog");

    osal_close_tmp_file();

    deb_stream = osal_file_open(deb_path, OS_NULL, &s, OSAL_STREAM_WRITE);
    if (deb_stream == OS_NULL) return s;

    return OSAL_SUCCESS;
}


osalStatus osal_program_device(
    os_char *buf,
    os_memsz buf_sz)
{
    os_memsz n_written;
    osalStatus s;

osal_debug_error("HERE prog");

    if (deb_stream == OS_NULL) {
        return OSAL_STATUS_FAILED;
    }
    s = osal_file_write(deb_stream, buf, buf_sz, &n_written, OSAL_STREAM_DEFAULT);
    if (s || n_written != buf_sz) {
        osal_cancel_device_programming();
        return s;
    }

    return OSAL_SUCCESS;
}

osalStatus osal_finish_device_programming(
    os_uint checksum)
{
    OSAL_UNUSED(checksum);
osal_debug_error("HERE finish prog");

    if (deb_stream == OS_NULL) {
        return OSAL_STATUS_FAILED;
    }
    osal_close_tmp_file();
    osal_install_package();

//        osal_reboot(0);

    return OSAL_SUCCESS;
}

void osal_cancel_device_programming(void)
{
osal_debug_error("HERE cancel prog");

    osal_close_tmp_file();
    osal_delete_tmp_file();
}


static void osal_close_tmp_file(void)
{
    if (deb_stream)
    {
        osal_file_close(deb_stream, OSAL_STREAM_DEFAULT);
        deb_stream = OS_NULL;
    }
}

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
    static os_char *const argv[] = {"dpkg", "-i", "--force-all", deb_path, ">pekka.txt", OS_NULL};
    // static os_char *const argv[] = {"ls", "-la", deb_path, ">pekka.txt", OS_NULL};
    OSAL_UNUSED(prm);

    osal_trace("program device: installer thread created");

    /* Let thread which created this one proceed.
     */
    osal_event_set(done);


    // dpkg -x iocomtempprog.deb /coderoot/production/


    // Switch user to root

    // sudo dpkg -i --force-all iocomtempprog.deb

    s = osal_create_process("dpkg", argv, 0);
    // s = osal_create_process("ls", argv, 0);
    if (s) {
        osal_debug_error("debian package installation failed");
    }

    while (1)
    {
        os_sleep(1000);
    }

    // osal_delete_tmp_file();
}
#endif


/**
****************************************************************************************************

  @brief Thread fuction to run the installation.
  @anchor osal_installer_thread

  The osal_installer_thread() function call "dpkg" to to install the debian package, and once
  ready restarts the application.

  @param   prm Pointer to parameters for new thread, pointer to end point object.
  @param   done Event to set when parameters have been copied to entry point
           functions own memory.

  @return  None.

****************************************************************************************************
*/
static void osal_install_package(void)
{
    osalThreadOptParams opt;
    os_memclear(&opt, sizeof(osalThreadOptParams));
    opt.thread_name = "endpoint";

    osal_thread_create(osal_installer_thread, OS_NULL,
        OS_NULL, OSAL_THREAD_DETACHED);
}


#endif
