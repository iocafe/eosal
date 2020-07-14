/**

  @file    net/common/osal_net_morse_code.h
  @brief   Morse codes from network state.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    15.7.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

struct osalNetworkState;

/** Enumeration of morse codes.
 */
typedef enum osalMorseCodeEnum
{
    MORSE_CONFIGURING = -2,
    MORSE_CONFIGURATION_MATCH = -1,
    MORSE_RUNNING = 0,
    MORSE_NETWORK_NOT_CONNECTED = 1, /* Not connected to WiFi or Ethernet network. */
    MORSE_LIGHTHOUSE_NOT_VISIBLE = 2,
    MORSE_NO_LIGHTHOUSE_FOR_THIS_IO_NETWORK = 3,
    MORSE_SECURITY_CONF_ERROR = 4,
    MORSE_NO_CONNECTED_SOCKETS = 5,
    MORSE_DEVICE_INIT_INCOMPLETE = 8,

    MORSE_UNKNOWN = 100
}
osalMorseCodeEnum;

/* Get morse code corresponding to network state.
 */
osalMorseCodeEnum osal_network_state_to_morse_code(
    struct osalNetworkState *net_state);
