/**

  @file    memory/common/osal_sysmem.h
  @brief   Operating system memory allocation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  Prototypes for operating system memory allocation functions.

  Copyright 2012 - 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifndef OSAL_SYSMEM_INCLUDED
#define OSAL_SYSMEM_INCLUDED

#if OSAL_MEMORY_MANAGER

/**
****************************************************************************************************

  @name Function types for operafing system memory allocation and release functions.

  The osal_sysmem_alloc() function allocates a memory block from operating system and
  osal_sysmem_free() releases it. The memory is allocated from operating system through
  the function pointers in osal_global structure. This code in DLLs to allocate
  and free memory through same functions as the process which loaded the DLL.

****************************************************************************************************
 */
/*@{*/

/* Allocate a block of memory.
 */
typedef os_char *osal_sysmem_alloc_func(
    os_memsz request_bytes,
    os_memsz *allocated_bytes);

/* Release a block of memory.
 */
typedef void osal_sysmem_free_func(
    void *memory_block,
    os_memsz bytes);

/*@}*/

#endif

/** 
****************************************************************************************************

  @name Operating System's Memory Allocation Functions

  Default functions for allocating and releasing memory. The osal_sysmem_alloc() function
  allocates a memory block from operating system and osal_sysmem_free() releases it.

****************************************************************************************************
 */
/*@{*/

/* Allocate a block of memory.
 */
os_char *osal_sysmem_alloc(
    os_memsz request_bytes,
    os_memsz *allocated_bytes);

/* Release a block of memory.
 */
void osal_sysmem_free(
    void *memory_block,
    os_memsz bytes);

/*@}*/

#endif
