/**

  @file    cpuid/common/osal_cpuid.h
  @brief   Get unique CPU or computer identifier.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    20.1.2020

  This is most useful for saving user login and password on PC computer or android device
  running user interface. Merging CPU id with  hard coded application key prevents snooping
  login information for discarded SSD drive, etc.

  This could be used to protect server's private key and other secret information. But this
  has a major down side: One cannot any more copy the server and assume that it will start
  working.

  This is rarely useful for microcontrollers, since flash is integrated to microcontroller.
  Thus attacker has access to both flash and microcontroller/board idenfifiation.

  Possible security improvement for servers and microcontrollers is to save additional random
  key code to USB flash, etc, removable media, or on a separate device in network.
  Thus removable media or network device would be the an key required to make server or device
  part of IO network. This approach has reliability and other downsides, which most often
  outweigh the added security. Thus taking care that decommissioned server SSD drives are wiped
  or destroyed, and security reset is done on decommissioned devices.

  This function can be critisized that it has limited entropy. This is true, an educated attacker
  could quess the possible combinations and simply try them all. We may improve entropy later on
  by adding mother board serial number, NIC address, etc to this function.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#pragma once
#ifndef OSAL_CPUID_H_
#define OSAL_CPUID_H_
#include "eosalx.h"

/* Merge CPU identifier to buffer with XOR.
 */
osalStatus osal_xor_cpuid(
    os_uchar *buf,
    os_memsz buf_sz);

#endif
