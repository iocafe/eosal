/**

  @file    persistent/arduino/osal_arduino_persistent.c
  @brief   Save persistent parameters on Arduino EEPROM.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.9.2019

  Arduino EEPROM api is used because it is well standardized. Hardware underneath can be flash,
  in case EEPROM omulation.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_PERSISTENT_SUPPORT
#include <Arduino.h>
#include <EEPROM.h>


/* Forward referred static functions.
 */


/**
****************************************************************************************************

  @brief Initialize persistent storage for use.
  @anchor os_persistent_initialze

  The os_persistent_initialze() function.

  @param   prm Pointer to parameters for persistent storage. For this implementation path
           member sets path to folder where to keep parameter files. Can be OS_NULL if not
           needed.
  @return  None.

****************************************************************************************************
*/
void os_persistent_initialze(
    osPersistentParams *prm)
{
}


/**
****************************************************************************************************

  @brief Load parameter block (usually structure) from persistent storage.
  @anchor os_persistent_load

  The os_persistent_load() function loads parameter structure identified by block number
  from the persistant storage. Load all parameters when micro controller starts, not during
  normal operation. If data cannot be loaded, the function leaves the block as is.

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   block Pointer to block (structure) to load.
  @param   block_sz Block size in bytes.
  @return  Number of bytes read. Zero if failed. Returned value maxes at block_sz.

****************************************************************************************************
*/
os_memsz os_persistent_load(
    osPersistentBlockNr block_nr,
    os_uchar *block,
    os_memsz block_sz)
{
    return 0;
}


/**
****************************************************************************************************

  @brief Save parameter block to persistent storage.
  @anchor os_persistent_save

  The os_persistent_save() function saves a parameter structure to persistent storage and
  identifies it by block number.

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   block Pointer to block (structure) to save.
  @param   block_sz Block size in bytes.
  @return  OSAL_SUCCESS indicates all fine, other return values indicate on error.

****************************************************************************************************
*/
osalStatus os_persistent_save(
    osPersistentBlockNr block_nr,
    os_uchar *block,
    os_memsz block_sz)
{
    return OSAL_SUCCESS;
}



#endif
