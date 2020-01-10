/**

  @file    memory/common/osal_memory.h
  @brief   Memory allocation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  This header file contains functions prototypes for OSAL memory allocation. The OSAL implements
  it's own memory manager.

  Cache line size: 64 bytes
  Now, the parity bits are for the use of the memory controller - so cache line size typically
  is 64 bytes. The processor really controls very little beyond the registers. Everything else
  going on in the computer is more about getting hardware in to optimize CPU performance.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#ifdef OSAL_WINDOWS
#include <memory.h>
#endif
#ifdef OSAL_LINUX
#include <memory.h>
#endif

#if OSAL_MEMORY_MANAGER

/**
****************************************************************************************************

  @name Memory Manager Defines

  These defines set memory manager operation: Maximum block size to handle and related block 
  table length, quick find table length for small memory blocks and base chunk size to allocate
  from operating system These defines are just default and can be overridden in osal_defs.h for
  the operating system or in project settings.

****************************************************************************************************
 */
/*@{*/

/** Limit for block size to be handled by memory manager. No memory blocks larger than this
    will be handled by memory manager, and actual limit will be little lower than this define.
	Larger block will be allocated by direct operating system calls.
 */
#ifndef OSAL_MEMORY_BLOCK_SZ_LIMIT
#define OSAL_MEMORY_BLOCK_SZ_LIMIT 0x40000000
#endif

/** Number of elements to allocate for memory block chains by size. 
 */
#ifndef OSAL_MEMORY_BLOCK_TABLE_LEN
#define OSAL_MEMORY_BLOCK_TABLE_LEN 75
#endif

/** Quick find array size. Quick find table is used quickly to convert number of bytes to 
    block size index for small memory blocks.
 */
#ifndef OSAL_MEMORY_QUICK_FIND_TABLE_LEN
#define OSAL_MEMORY_QUICK_FIND_TABLE_LEN 200
#endif

/** Base chunk size to allocate from operating system. Actual OS allocation request is rounded
    to be dividable by memory block size for small memory blocks, and memory block size for
	larger ones.
 */
#ifndef OSAL_MEMORY_CHUNK_SIZE
#define OSAL_MEMORY_CHUNK_SIZE 2000
#endif

/*@}*/


/**
****************************************************************************************************

  @name Memory Manager's internal structures.

  These internal structures are used by OSAL memory manager to manage memory it has allocated
  from the operating system.

****************************************************************************************************
 */
/*@{*/

/** Operating system memory chunk list item. This structure is allocated at beginning of chunk
    to maintain list of allocated chunks, and is needed only to clean up allocated memory
    when the process exits or restarts OSAL.
 */
#if OSAL_PROCESS_CLEANUP_SUPPORT
typedef struct osalMemoryChunkHeader
{
    /** Next in chain of OS memory blocks reserved for this size of items
     */
    struct osalMemoryChunkHeader *next_chunk;
}
osalMemoryChunkHeader;
#endif

/** Structure for slicing up the chunk of memory. When a chunk of memory is allocated,
    information of unused space in chunk is stored into slicing sturcture, and linked
    to list of same size group (by remining bytes).
 */
typedef struct osalMemorySliceHeader
{
    /** Next slice in list of slices in same size group. first_slice in osalMemManagerState
        state structure points this.
     */
    struct osalMemorySliceHeader *next_slice;

    /** Number of bytes left from the chunk allocated from the operating system.
        This includes space used by the osalMemorySliceHeader structure.
     */
    os_memsz bytes_left;
}
osalMemorySliceHeader;

/** Memory manager state structure.
 */
typedef struct
{
    /** First in list of free memory blocks of specific size.
     */
    void *first_free_block[OSAL_MEMORY_BLOCK_TABLE_LEN];

    /** Size of memory block by size group index.
     */
    os_memsz block_sz[OSAL_MEMORY_BLOCK_TABLE_LEN];

    /** Number of used size groups.
     */
    os_int n;

    /** Size of largest memory block handled by OSAL memory manager. Larger blocks
        are handled by direct operating system calls.
     */
    os_memsz max_block_sz;

    /** For small memory blocks: Array index is number of bytes and value is size
        group index.
     */
    os_uchar quick_find[OSAL_MEMORY_QUICK_FIND_TABLE_LEN];

    /** Slicing chunks allocated from operating system.
     */
    osalMemorySliceHeader *first_slice[OSAL_MEMORY_BLOCK_TABLE_LEN];

#if OSAL_PROCESS_CLEANUP_SUPPORT
    /** List of memory chunks allocated from operating system. This is used only to be able
        to release memory when process exists or restarts OSAL.
     */
    osalMemoryChunkHeader *chunk_list;
#endif

}
osalMemManagerState;

/*@}*/

/**
****************************************************************************************************

  @name Memory Manager Initialization and Shut Down Functions

  These are called internally by OSAL, and should not normally be called by application.
  The osal_initialize() calls osal_memory_initialize() and osal_shutdown() calls
  osal_memory_shutdown().

****************************************************************************************************
 */
/*@{*/

/* Initialize memory management.
 */
void osal_memory_initialize(
    void);

/* Shut down memory management.
 */
void osal_memory_shutdown(
    void);

/*@}*/


/** 
****************************************************************************************************

  @name Memory Allocation Functions

  The osal_memory_allocate() function allocates a memory block and osal_memory_free() releases it.

****************************************************************************************************
 */
/*@{*/

/* Application memory allocation functions. These can be remapped for DLLs, etc.
 */
#define os_malloc(r,a) osal_memory_allocate(r,a)
#define os_free(b,s) osal_memory_free(b,s)

/* Allocate a block of memory.
 */
os_char *osal_memory_allocate(
    os_memsz request_bytes,
    os_memsz *allocated_bytes);

/* Release a block of memory.
 */
void osal_memory_free(
    void *memory_block,
    os_memsz bytes);

/*@}*/

#elif OSAL_DYNAMIC_MEMORY_ALLOCATION==0

#define os_malloc(r,a) osal_memory_allocate_static_block(r,a)
#define os_free(b,s) osal_memory_free_static_block(b,s)

#else
#define os_malloc(r,a) osal_sysmem_alloc(r,a)
#define os_free(b,s) osal_sysmem_free(b,s)

#endif /* OSAL_MEMORY_MANAGER */


#if OSAL_DYNAMIC_MEMORY_ALLOCATION==0

/* Structure to chain free static memory blocks.
 */
typedef struct osalStaticMemBlock
{
    /** Pointer to next static memory block in list.
     */
    struct osalStaticMemBlock * next;
    os_int block_sz;
}
osalStaticMemBlock;

/* List of prepared free static memory blocks.
 */
extern osalStaticMemBlock *osal_static_mem_block_list;

/* Add static memory block to be allocated by osal_memory_allocate_static_block (os_malloc macro).
 */
void osal_memory_add_static_block(
    void *block,
    os_int block_sz,
    os_int block_alloc);

/* Reserved a prepared static memory block.
 */
os_char *osal_memory_allocate_static_block(
    os_memsz request_bytes,
    os_memsz *allocated_bytes);

/* Release a static memory block.
 */
void osal_memory_free_static_block(
    void *memory_block,
    os_memsz bytes);

#endif


/**
****************************************************************************************************

  @name Memory Manipulation Functions

  The os_memclear() function clears and the os_memcpy() function copies a range of 
  memory.

****************************************************************************************************
 */
/*@{*/

#ifdef OSAL_WINDOWS
  #define OSAL_MEMORY_OS_CLR_AND_CPY
  #define os_memclear(d,c) memset((d),0,(size_t)(c))
  #define os_memcpy(d,s,c) memcpy((d),(s),(size_t)(c))
  #define os_memcmp(d,s,c) memcmp((d),(s),(size_t)(c))
  #define os_memmove(d,s,c) memmove((d),(s),(size_t)(c))
#endif

#ifdef OSAL_LINUX
  #define OSAL_MEMORY_OS_CLR_AND_CPY
  #define os_memclear(d,c) memset((d),0,(size_t)(c))
  #define os_memcpy(d,s,c) memcpy((d),(s),(size_t)(c))
  #define os_memcmp(d,s,c) memcmp((d),(s),(size_t)(c))
  #define os_memmove(d,s,c) memmove((d),(s),(size_t)(c))
#endif

#ifndef OSAL_MEMORY_OS_CLR_AND_CPY
/* Clear memory.
 */
void os_memclear(
    void *dst,
    os_memsz count);

/* Copy memory block.
 */
void os_memcpy(
    void *dst,
    const void *src,
    os_memsz count);

/* Copy memory block.
 */
os_int os_memcmp(
    const void *str1,
    const void *str2,
    os_memsz count);

/* Move memory block.
 */
void os_memmove(
    void *dst,
    const void *src,
    os_memsz count);

#endif

/*@}*/
