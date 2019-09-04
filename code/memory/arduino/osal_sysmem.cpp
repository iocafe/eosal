/**

  @file    memory/arduino/osal_sysmem.c
  @brief   Operating system memory allocation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Linux operating system memory allocation functions.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#include <stdlib.h>


/**
****************************************************************************************************

  @brief Allocate a memory block from OS.
  @anchor osal_sysmem_alloc

  The osal_sysmem_alloc()...

  HeapAlloc allocates at least the amount of memory requested. HeapSize function is used to
  determine actual size of the allocated block.

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
    if (allocated_bytes) *allocated_bytes = request_bytes;
    return (os_char*)malloc((size_t)request_bytes);
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
    free(memory_block);
}
