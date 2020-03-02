/**

  @file    socket/common/osal_shared_net_info.h
  @brief   Shared network information.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    1.3.2020

  Shared osalSocketGlobal strcture holds information about network adapters and wifi
  for OSAL sockets. This stucture is shared by several omplmenentations of socket wrappers
  and network initialization implementation. The main function is to define common format
  how to pass information from initialization code to the socket wrapper.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
typedef struct osalSocketNicInfo
{
    /** Network address, like "192.168.1.220".
     */
    os_char ip_address[OSAL_IPADDR_SZ];

    /** OS_TRUE to enable sending UDP multicasts trough this network interface.
     */
    os_boolean send_udp_multicasts;

    /** OS_TRUE to receiving UDP multicasts trough this NIC.
     */
    os_boolean receive_udp_multicasts;
}
osalSocketNicInfo;

/* Global data for sockets.
 */
typedef struct osalSocketGlobal
{
    osalSocketNicInfo nic[OSAL_MAX_NRO_NICS];
    os_int n_nics;
}
osalSocketGlobal;


