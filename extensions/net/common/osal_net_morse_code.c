/**

  @file    net/common/osal_net_morse_code.c
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
#include "eosalx.h"

/**
****************************************************************************************************

  @brief Get morse code corresponding to network state.
  @anchor osal_network_state_to_morse_code

  The osal_network_state_to_morse_code() function examines network state structure and selects which
  morse code best describes it.

  @param   net_state Network state structure.
  @return  Morse code enumeration value.

****************************************************************************************************
*/
osalMorseCodeEnum osal_network_state_to_morse_code(
    struct osalNetworkState *net_state)
{
    osalMorseCodeEnum code;
    osaLightHouseClientState lighthouse_state;
    osalGazerbeamConnectionState gbs;

    /* If Gazerbeam configuration (WiFi with Android phone) is on?
     */
    gbs = osal_get_network_state_int(OSAL_NS_GAZERBEAM_CONNECTED, 0);
    if (gbs)
    {
        code = (gbs == OSAL_NS_GAZERBEAM_CONFIGURATION_MATCH)
                ? MORSE_CONFIGURATION_MATCH : MORSE_CONFIGURING;
        goto setit;
    }

    /* If we are programming the flash.
     */
    if (osal_get_network_state_int(OSAL_NS_PROGRAMMING_DEVICE, 0)) {
        code = MORSE_PROGRAMMING_DEVICE;
        goto setit;
    }

    /* If WiFi is not connected?
     */
    if (osal_get_network_state_int(OSAL_NS_NETWORK_USED, 0) &&
        !osal_get_network_state_int(OSAL_NS_NETWORK_CONNECTED, 0))
    {
        code = MORSE_NETWORK_NOT_CONNECTED;
        goto setit;
    }

    /* Check for light house.
     */
    lighthouse_state
        = (osaLightHouseClientState)osal_get_network_state_int(OSAL_NS_LIGHTHOUSE_STATE, 0);
    if (lighthouse_state != OSAL_LIGHTHOUSE_NOT_USED &&
        lighthouse_state != OSAL_LIGHTHOUSE_OK)
    {
        code = (lighthouse_state == OSAL_LIGHTHOUSE_NOT_VISIBLE)
            ? MORSE_LIGHTHOUSE_NOT_VISIBLE : MORSE_NO_LIGHTHOUSE_FOR_THIS_IO_NETWORK;
        goto setit;
    }

    /* Certificates/keys not loaded.
     */
    if (/* osal_get_network_state_int(OSAL_NS_SECURITY_CONF_ERROR, 0) || */
        osal_get_network_state_int(OSAL_NS_NO_CERT_CHAIN, 0))
    {
        code = MORSE_SECURITY_CONF_ERROR;
        goto setit;
    }

    /* If no connected sockets?
     */
    if (osal_get_network_state_int(OSAL_NRO_CONNECTED_SOCKETS, 0) == 0)
    {
        code = MORSE_NO_CONNECTED_SOCKETS;
        goto setit;
    }

    /* If device initialization is incomplete?
     */
    if (osal_get_network_state_int(OSAL_NS_DEVICE_INIT_INCOMPLETE, 0))
    {
        code = MORSE_DEVICE_INIT_INCOMPLETE;
        goto setit;
    }

    /* All running fine.
     */
    code = MORSE_RUNNING;

setit:
    return code;
}
