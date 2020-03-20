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


#if OSAL_DYNAMIC_MEMORY_ALLOCATION
/**
****************************************************************************************************

  @brief Load or access persistent memory block.

  The os_load_persistent_malloc function loads memory block into buffer. If the data on flash
  memory can be accessed directly, the function just returns pointer to it, much like
  the os_persistent_get_ptr() function. If data must be loaded from file system, etc
  memory is allocated for it and the function returns OSAL_MEMORY_ALLOCATED.
  In latter case memory must be freed.

  - Alloctes memory for block. If block is set to non NULL, it must be released by os_free().
  - Reads also blocks flagged secret.

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   pblock Block pointer to set.
  @param   pblock_sz Pointer to integer where to store block size.
  @return  If successfull, the function return OSAL_SUCCESS or OSAL_MEMORY_ALLOCATED.
           If the function returns OSAL_MEMORY_ALLOCATED, memory for the block
           oas been allocated by os_malloc and has to be freed by os_free().

****************************************************************************************************
*/
osalStatus os_load_persistent_malloc(
    osPersistentBlockNr block_nr,
    os_char **pblock,
    os_memsz *pblock_sz)
{
    const os_char *block;
    os_char *p;
    os_memsz block_sz, n_read;
    osPersistentHandle *h = OS_NULL;
    osalStatus s;

    *pblock = OS_NULL;
    *pblock_sz = 0;

    /* If persistant storage is in micro-controller's flash, we can just get pointer to data block
       and data size.
     */
    s = os_persistent_get_ptr(block_nr, &block, &block_sz, OSAL_PERSISTENT_SECRET);
    if (s == OSAL_SUCCESS && block_sz > 0)
    {
        *pblock = (os_char*)block;
        *pblock_sz = block_sz;
        return OSAL_SUCCESS;
    }

    /* No success with direct pointer to flash, try loading from persisten storage.
     */
    h = os_persistent_open(block_nr, &block_sz, OSAL_PERSISTENT_READ|OSAL_PERSISTENT_SECRET);
    if (h == OS_NULL || block_sz <= 0)
    {
        return OSAL_STATUS_FAILED;
    }

    /* Allocate memory.
     */
    p = os_malloc(block_sz, OS_NULL);
    if (p == OS_NULL)
    {
        os_persistent_close(h, OSAL_PERSISTENT_DEFAULT);
        return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;
    }

    /* Read the block.
     */
    n_read = os_persistent_read(h, p, block_sz);
    if (n_read != block_sz)
    if (p == OS_NULL)
    {
        os_free(p, block_sz);
        os_persistent_close(h, OSAL_PERSISTENT_DEFAULT);
        return OSAL_STATUS_FAILED;
    }

    /* Set the block content and size pointers, success.
     */
    os_persistent_close(h, OSAL_PERSISTENT_DEFAULT);
    *pblock = p;
    *pblock_sz = block_sz;
    return OSAL_MEMORY_ALLOCATED;
}
#endif


/**
****************************************************************************************************

  @brief Load known persistent block of known size into buffer.

  The os_load_persistent function floats known size persistent block into buffer given as argument.
  @return  If successfull, the function return OSAL_SUCCESS. Other values indicate an error.

****************************************************************************************************
*/
osalStatus os_load_persistent(
    osPersistentBlockNr block_nr,
    os_char *block,
    os_memsz block_sz)
{
    const os_char *sblock;
    os_memsz sblock_sz, n_read;
    osPersistentHandle *h;
    osalStatus s;

    /* In case of errors.
     */
    os_memclear(block, block_sz);

    /* If persistant storage is in micro-controller's flash, we can just get pointer to data block
       and data size.
     */
    s = os_persistent_get_ptr(block_nr, &sblock, &sblock_sz, OSAL_PERSISTENT_SECRET);
    if (s == OSAL_SUCCESS && sblock_sz == block_sz)
    {
        os_memcpy(block, sblock, block_sz);
        return OSAL_SUCCESS;
    }

    /* No success with direct pointer to flash, try loading from persisten storage.
     */
    h = os_persistent_open(block_nr, &sblock_sz, OSAL_PERSISTENT_READ|OSAL_PERSISTENT_SECRET);
    if (h == OS_NULL) return OSAL_STATUS_FAILED;

    s = OSAL_STATUS_FAILED;
    if (sblock_sz == block_sz)
    {
        n_read = os_persistent_read(h, block, block_sz);
        if (n_read == block_sz) s = OSAL_SUCCESS;
    }
    os_persistent_close(h, OSAL_PERSISTENT_DEFAULT);
    return s;
}

#endif
