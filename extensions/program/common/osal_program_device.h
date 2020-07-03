/**

  @file    eosal/extensions/program/linux/osal_program_device.h
  @brief   Write IO device firmware to flash.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    1.7.2020

  The ESP32 comes with ready software update API, called OTA. The OTA update mechanism allows
  a device to update itself over IOCOM connection while the normal firmware is running.

  OTA requires configuring the Partition Table of the device with at least two “OTA app slot”
  partitions (ieota_0 and ota_1) and an “OTA Data Partition”. The OTA operation functions write
  a new app firmware image to whichever OTA app slot is not currently being used for booting.
  Once the image is verified, the OTA Data partition is updated to specify that this image
  should be used for the next boot.

  - https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/ota.html
  - https://github.com/scottchiefbaker/ESP-WebOTA/tree/master/src

  ESP-IDF: Copyright (C) 2015-2019 Espressif Systems. This source code is licensed under
  the Apache License 2.0.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#if OSAL_DEVICE_PROGRAMMING_SUPPORT

/* DO WE NEED SOME HANDLE STRUCTURE FOR LINUX AND WINdOWS, WE MAY BE RUNNING MULTIPLE INSTALLS, MAYBE NO NEED TO SUPPORT
 *
 * HOW ABOUT INSTALLATION FILE NAME, LINUX WINDOWS. DO WE SUPPORT JUST COPING A BINARY OR DEB, ETC PACKAGE
 *
 * LINUX/WINDOWS: STARTUP SERVER, START SCRIPTS AND BINARIES IN SPECIFIC FOLDER? PYTHON?
  typedef struct
{

}
programmingHandle;
*/

void osal_start_device_programming(void);

void osal_program_device(
    os_char *buf,
    os_memsz buf_sz);

void osal_finish_device_programming(
    os_uint checksum);

void osal_cancel_device_programming(void);

#endif
