/**

  @file    memory/esp32/osal_esp32_sysmem.c
  @brief   Operating system memory allocation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Esp-idf operating system memory allocation functions.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#ifdef OSAL_ESP32
#include <stdlib.h>

/**
****************************************************************************************************

  @brief Allocate a memory block from OS.
  @anchor osal_sysmem_alloc

  The osal_sysmem_alloc() is light weight interface to OS memory allocation. It doesn't use
  mutex locks, etc. 

  @param   request_bytes The function allocates at least the amount of memory requested by this
           argument.
  @param   allocated_bytes Pointer to long integer into which to store the actual size of the
           allocated block (in bytes). The actual size is greater or equal to requested size.
           If actual size is not needed, this parameter can be set to OS_NULL.

  @return  Pointer to the allocated memory block, or OS_NULL if the function failed (out of
           memory).

****************************************************************************************************
*/
os_char *osal_sysmem_alloc(
    os_memsz request_bytes,
    os_memsz *allocated_bytes)
{
    os_char *mem;

    if (allocated_bytes) {
        *allocated_bytes = request_bytes;
    }

    mem = (os_char*)malloc((size_t)request_bytes);
    if (mem == NULL) {
        osal_debug_error("osal_sysmem_alloc failed");
        osal_error(OSAL_SYSTEM_ERROR, eosal_mod, OSAL_STATUS_MEMORY_ALLOCATION_FAILED, OS_NULL);
        return NULL;
    }

    osal_resource_monitor_update(OSAL_RMON_SYSTEM_MEMORY_ALLOCATION, request_bytes);
    return mem;
}


/**
****************************************************************************************************

  @brief Release a memory block back to OS.
  @anchor osal_sysmem_free

  The osal_sysmem_free() function releases a block of memory allocated from operating system
  by osal_sysmem_alloc() function.

  @param   memory_block Pointer to memory block to release. If this pointer is OS_NULL, then
           the function does nothing.
  @param   bytes Either requested or allocated size of memory block.

  @return  None.

****************************************************************************************************
*/
void osal_sysmem_free(
    void *memory_block,
    os_memsz bytes)
{
    if (memory_block) {
        free(memory_block);
        osal_resource_monitor_update(OSAL_RMON_SYSTEM_MEMORY_ALLOCATION, -bytes);
    }
}

#endif
