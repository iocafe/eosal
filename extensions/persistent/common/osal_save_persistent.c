/**

  @file    persistent/common/osal_load_persistent.c
  @brief   Load persistent block into memory.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    30.1.2020

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_PERSISTENT_SUPPORT

/**
****************************************************************************************************

  @brief Save persistent block.

  The ioc_save_block function write a memory block from buffer to persistent storage.

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   block Pointer to block (structure) to save.
  @param   block_sz Block size in bytes.
  @param   delete_block OS_TRUE to write a block with empty content.
  @return  If successfull, the function return OSAL_SUCCESS. Other values indicate an error.

****************************************************************************************************
*/
osalStatus ioc_save_block(
    osPersistentBlockNr block_nr,
    const os_char *block,
    os_memsz block_sz,
    os_boolean delete_block)
{
    osPersistentHandle *h;
    osalStatus s;

    h = os_persistent_open(block_nr, OS_NULL, OSAL_PERSISTENT_WRITE|OSAL_PERSISTENT_SECRET);
    if (h == OS_NULL) return OSAL_STATUS_FAILED;

    if (delete_block)
    {
        s = os_persistent_write(h, "", 0);
    }
    else
    {
        s = os_persistent_write(h, block, block_sz);
    }
    os_persistent_close(h, OSAL_PERSISTENT_DEFAULT);
    return s;
}

#endif
