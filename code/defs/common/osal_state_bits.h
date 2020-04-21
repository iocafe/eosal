/**

  @file    defs/common/osal_state_bits.h
  @brief   State bits for IO, etc.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  These are IO signal state bits, and defined as part of OSAL for interoperability of
  several libraries that use these.

  Signal state bits
  It is not enough to know the value of a temperature sensor signal (from IO device),
  speed setting for a motor (to IO device) or if some binary input is ON or OFF when the device.

  Along the temperature value, let’s say 30 degrees Celsius, we need to pass information
  that the signal “has value/is connected” and the sensor is not broken. This type of
  information is called signal state byte, and we should have one for every signal.

  Bits in signal state byte
  Signal byte contains signal state bits as “----OYCX”.
    - bit O, OSAL_STATE_ORANGE = 8
    - bit Y, OSAL_STATE_YELLOW = 4
    - bit C, OSAL_STATE_CONNECTED = 2
    - bit X,  OSAL_STATE_BOOLEAN_VALUE = 1
    - bits marked with ‘-’ are reserved for future uses (redundancy, etc).

  OSAL_STATE_CONNECTED. There are times when controller doesn’t know state of input on IO board,
  or IO board doesn’t know weather controller wants some output to be set ON or OFF.
  This happens for example when IO device connects to controller, or either IO device or
  controller is restarted, network is temporary disabled… Also connections can be chained,
  so that signal needs to jump trough a few hoops, before it is known somewhere else.
  The connected bit is set to 1 along with signal value.

  OSAL_STATE_ORANGE and OSAL_STATE_YELLOW bits. These are for reporting broken or untrusted
  hardware. Total failure is indicated by OSAL_STATE_RED = 12, which is simply both
  OSAL_STATE_YELLOW and OSAL_STATE_ORANGE bits set. For example if temperature sensor input
  has value 0 or 4095 (for 12 bit A/D) it can be quite safely assumed that it is not connected
  and IO device can set OSAL_STATE_ORANGE for it. Mask  OSAL_STATE_ERR_MASK is also defined
  with both error/warning bits set.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

#define OSAL_STATE_YELLOW 8
#define OSAL_STATE_ORANGE 4
#define OSAL_STATE_RED (OSAL_STATE_ORANGE|OSAL_STATE_YELLOW)
#define OSAL_STATE_ERROR_MASK (OSAL_STATE_ORANGE|OSAL_STATE_YELLOW)
#define OSAL_STATE_UNCONNECTED 0
#define OSAL_STATE_CONNECTED 2
#define OSAL_STATE_BOOLEAN_VALUE 1
