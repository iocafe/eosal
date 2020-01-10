/**

  @file    memory/commmon/osal_memory.c
  @brief   Memory allocation manager.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Memory allocation manager.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosal.h"

#if OSAL_MEMORY_MANAGER

/** Value used to mark word just before memory block when debugging memory use.
 */
#define OSAL_MEMORY_BLOCK_START_MARK 0x6EC9

/** Value used to mark word just after memory block when debugging memory use.
 */
#define OSAL_MEMORY_BLOCK_END_MARK 0x6EC1


/* Forward referred static functions.
 */
static os_short osal_memory_get_block_ix(
    os_memsz bytes);

static void *osal_memory_allocate_slice(
    os_int ix);


/**
****************************************************************************************************

  @brief Initialize OSAL memory manager.
  @anchor osal_memory_initialize

  The osal_memory_initialize() function initializes OSAL memory manager. This function is called by
  osal_initialize() and should not normally be called by application.

  The OSAL memory manager is used only if OSAL_MEMORY_MANAGER define is nonzero. If the
  OSAL_MEMORY_MANAGER define is zero, osal_memory_allocate() and osal_memory_free() functions
  call operating system's memory management functions directly.

  @return  None.

****************************************************************************************************
*/
void osal_memory_initialize(
    void)
{
   void
        **first_free_block;

   osalMemorySliceHeader
        **first_slice;

   os_memsz
        bytes,
        step,
        *block_sz;

   os_int
        n,
        i,
        j;

   os_uchar
        *quick_find;

    /* Set pointers to default operating system memory allocation and release functions.
     */
    osal_global->sysmem_alloc_func = osal_sysmem_alloc;
    osal_global->sysmem_free_func = osal_sysmem_free;

    /* Get pointers to arrays, no entries yet.
     */
    first_free_block = osal_global->memstate.first_free_block;
    first_slice = osal_global->memstate.first_slice;
    block_sz = osal_global->memstate.block_sz;
    quick_find = osal_global->memstate.quick_find;
    n = 0;

    /* Store optimized block sizes.
     */

    /* Add incremental block sizes to chain table.
     */
    for (bytes = sizeof(void*);
         bytes <= OSAL_MEMORY_BLOCK_SZ_LIMIT;
         bytes += step)
    {
        /* If too many entries
         */
        if (n >= OSAL_MEMORY_BLOCK_TABLE_LEN)
        {
#if OSAL_MEMORY_DEBUG
            /* If this warning occurs, the OSAL_MEMORY_BLOCK_TABLE_LEN is
               too small to compared to OSAL_MEMORY_BLOCK_SZ_LIMIT.
             */
            osal_debug_error("Out of block table");
#endif
            break;
        }

        /* Make sure that list pointers are NULL.
         */
        first_free_block[n] = OS_NULL;
        first_slice[n] = OS_NULL;

        /* Save entry for the block size
         */
        block_sz[n] = bytes;
        n++;

        /* Calculate step to next size.
         */
        step = (bytes / 3 + sizeof(void*) - 1) / sizeof(void*);
        step *= sizeof(void*);
    }

#if OSAL_DEBUG
#if OSAL_MEMORY_DEBUG
    if (4*n/3 < OSAL_MEMORY_BLOCK_TABLE_LEN)
    {
        /* If this warning occurs, the OSAL_MEMORY_BLOCK_TABLE_LEN is
           unnecessarily large compared to OSAL_MEMORY_BLOCK_SZ_LIMIT.
         */
        osal_debug_error("Block table too large");
    }
#endif
#endif

    /* Sort entries.
     */

    /* Merge duplicate or very close entries.
     */

#if OSAL_MEMORY_DEBUG
    /* Add space for check data.
     */
    for (i = 0; i < n; ++i)
    {
        block_sz[i] += 4 * sizeof(os_short);
    }
#endif

    /* Fill in quick find table for small memory block. This
       allows very quickly to determine right chain for the
       small memory blocks.
     */
    for (i = 0, j = 0; i < OSAL_MEMORY_QUICK_FIND_TABLE_LEN; i++)
    {
        if (i > block_sz[j]) j++;
        quick_find[i] = (os_uchar)j;
    }

    /* Save number of used block sizes and maximum block size handled by memory manager.
     */
    osal_global->memstate.n = n;
    osal_global->memstate.max_block_sz = block_sz[n-1];
}


#if OSAL_PROCESS_CLEANUP_SUPPORT
/**
****************************************************************************************************

  @brief Shut down memory management.
  @anchor osal_memory_shutdown

  The osal_memory_shutdown() shuts down OSAL memory management and releases any resources
  allocated by memory management. This function is called by osal_shutdown() and should 
  not normally be called by application.

  @return  None.

****************************************************************************************************
*/
void osal_memory_shutdown(
    void)
{
    osalMemoryChunkHeader
        *chunk_hdr,
        *next_chunk_hdr;

	/* Loop through all memory chunks allocated from the operating system.
	 */
    for (chunk_hdr = osal_global->memstate.chunk_list;
         chunk_hdr;
         chunk_hdr = next_chunk_hdr)
    {
        next_chunk_hdr = chunk_hdr->next_chunk;

        /* Release memory back to operating system.
         */
        osal_global->sysmem_free_func(chunk_hdr, 0 /* size not needed for normal free */ );
    }
}
#endif


/**
****************************************************************************************************

  @brief Allocate a block of memory.
  @anchor osal_memory_allocate

  The osal_memory_allocate()...

  @param   request_bytes The function allocates at least the amount of memory requested by this
           argument.
  @param   allocated_bytes Pointer to long integer into which to store the actual size of the
           allocated block (in bytes). The actual size is greater or equal to requested size.
           If actual size is not needed, this parameter can be set to OS_NULL.

  @return  Pointer to the allocated memory block.

****************************************************************************************************
*/
os_char *osal_memory_allocate(
    os_memsz request_bytes,
    os_memsz *allocated_bytes)
{
    os_short
        ix;

    os_char
        *memory_block;

#if OSAL_MEMORY_DEBUG
	os_memsz
		my_block_sz;

    request_bytes += 4 * sizeof(os_short);
#endif

    /* Use quick find table to find block size group for the small memory blocks.
     */
    if (request_bytes < OSAL_MEMORY_QUICK_FIND_TABLE_LEN)
    {
        ix = osal_global->memstate.quick_find[request_bytes];
    }

    /* Larger block, still handled by memory manager, use function to find block size index.
     */
    else if (request_bytes <= osal_global->memstate.max_block_sz)
    {
        ix = osal_memory_get_block_ix(request_bytes);
    }

    /* Too large block for memory manager, call operating system directly to allocate memory.
     */
    else
    {
#if OSAL_MEMORY_DEBUG
        memory_block = osal_global->sysmem_alloc_func(request_bytes, &my_block_sz);
        if (allocated_bytes) *allocated_bytes = my_block_sz - 4 * sizeof(os_short);
		ix = 0;
#else
        memory_block = osal_global->sysmem_alloc_func(request_bytes, allocated_bytes);

#endif
		if (memory_block == OS_NULL)
        {
            osal_debug_error("Out of memory for large blocks");
		    return OS_NULL;
        }
		goto return_memory_block;
    }

	/* Lock system mutex.
	 */
	os_lock();

    /* Get pointer to fist free memory block of this size from chain of free blocks.
     */
    memory_block = osal_global->memstate.first_free_block[ix];

    /* If we do not have free block of this size in immediate chain, then allocate
       slice from operating system chunk. If operating system memory allocation
       fails, return OS_NULL.
     */
    if (memory_block == OS_NULL)
    {
        memory_block = osal_memory_allocate_slice(ix);
        if (memory_block == OS_NULL) 
		{
			os_unlock();
			osal_debug_error("Out of memory");
			return OS_NULL;
		}
    }

    /* Detach from chain of free blocks of this size.
     */
    else
    {
        osal_global->memstate.first_free_block[ix] = *((void**)memory_block);
    }

	/* Unlock system mutex.
	 */
	os_unlock();

#if OSAL_MEMORY_DEBUG
	my_block_sz = osal_global->memstate.block_sz[ix];
#endif

    /* Return allocated block size if needed.
     */
    if (allocated_bytes)
    {
#if OSAL_MEMORY_DEBUG
        *allocated_bytes = my_block_sz - 4*sizeof(os_short);
#else
        *allocated_bytes = osal_global->memstate.block_sz[ix];
#endif
    }


return_memory_block:


#if OSAL_MEMORY_DEBUG
	/* Store debug information. This tags word just before memory block with application
	   with OSAL_MEMORY_BLOCK_START_MARK and word just after with OSAL_MEMORY_BLOCK_END_MARK.
	   These are compared when memory block is released for buffer overruns. The size
	   index is stored into the first word. This is checked when memory block is
	   freed that bytes argument is correct.
	 */
	*(os_short*)memory_block = ix;
	*(os_short*)(memory_block + sizeof(os_short)) = OSAL_MEMORY_BLOCK_START_MARK;
	*(os_short*)(memory_block + my_block_sz - 2*sizeof(os_short)) = OSAL_MEMORY_BLOCK_END_MARK;

	/* Return pointer to memory block (starting from first byte which should be visible to
	   calling application.
	 */
    return memory_block + 2*sizeof(os_short);
#else
	/* Return pointer to the memory block.
	 */
    return memory_block;
#endif
}


/**
****************************************************************************************************

  @brief Release a block of memory.
  @anchor osal_memory_free

  The osal_memory_free() function releases a block of memory allocated from operating system
  by osal_memory_allocate() function.

  @param   memory_block Pointer to memory block to release. If this pointer is OS_NULL, then
           the function does nothing.
  @param   bytes Size of memory block, either request_bytes given as argument or allocated_bytes
		   returned by osal_memory_allocate() function.

  @return  None.

****************************************************************************************************
*/
void osal_memory_free(
    void *memory_block,
    os_memsz bytes)
{
    os_short
        ix;

    /* NULL memory block is accepted, function just does nothing.
     */
    if (memory_block == OS_NULL) return;

    /* In debug mode allow space for debug information.
     */
#if OSAL_MEMORY_DEBUG
    bytes += 4 * sizeof(os_short);
    memory_block = (os_char*)memory_block - 2 * sizeof(os_short);
#endif

    /* Use quick find table to find block index for the small memory blocks.
     */
    if (bytes < OSAL_MEMORY_QUICK_FIND_TABLE_LEN)
    {
        ix = osal_global->memstate.quick_find[bytes];
    }

    /* Larger block, use function to find block index.
     */
    else if (bytes <= osal_global->memstate.max_block_sz)
    {
        ix = osal_memory_get_block_ix(bytes);
    }

    /* Too large block for memory manager, call operating system directly.
     */
    else
    {
        osal_global->sysmem_free_func(memory_block, bytes);
        return;
    }

#if OSAL_MEMORY_DEBUG
	if (*(os_short*)((os_char*)memory_block + sizeof(os_short))
		!= OSAL_MEMORY_BLOCK_START_MARK)
	{
		osal_debug_error("Memory corrupted 1");
	}

	if (ix != *(os_short*)memory_block)
	{
		osal_debug_error("bytes given to osal_memory_free is faulty");
	}

	if (*(os_short*)((os_char*)memory_block + osal_global->memstate.block_sz[ix] 
		- 2*sizeof(os_short)) != OSAL_MEMORY_BLOCK_END_MARK)
	{
		osal_debug_error("Memory corrupted 2");
	}
#endif

	/* Synchronize.
	 */
	os_lock();

    /* Join to chain of free blocks of the same size.
     */
    *((void**)memory_block) = osal_global->memstate.first_free_block[ix];
    osal_global->memstate.first_free_block[ix] = memory_block;

	/* End synchronization.
	 */
	os_unlock();
}


/**
****************************************************************************************************

  @brief Find index of chain of memory blocks.

  The osal_memory_get_block_ix() function finds index of chain of specific size of blocks.
  The chain index returned will be chain of smallest blocks, which can hold number of memory 
  bytes given as argument.

  @param   bytes Number of bytes in the block.

  @return  Block chain index.

****************************************************************************************************
*/
static os_short osal_memory_get_block_ix(
    os_memsz bytes)
{
   os_memsz
        *block_sz;

   os_int
        n;

   os_short
        f,
        l,
        c;

    /* Get pointers to block size by chain index array and number of items in it.
     */
    block_sz = osal_global->memstate.block_sz;
    n = osal_global->memstate.n;

    /* First and last possible index in the array are:
     */
    f = 0;
    l = n-1;

    /* Check for chain of smallest memory blocks which is used for blocks
       of equal or larger size than bytes given as argument
     */
    do
    {
        c = ((f + l) >> 1);
        if (bytes <= block_sz[c]) l = c;
        else f = c+1;
    }
    while (f != l);

    /* Return block chain index.
     */
    return f;
}


/**
****************************************************************************************************

  @brief Find index of chain of memory slices.

  The osal_memory_get_slice_ix() function finds index of chain of specific size of blocks.

  @param   bytes Number of bytes in the slice.

  @return  Block chain index.

****************************************************************************************************
*/
static os_short osal_memory_get_slice_ix(
    os_memsz bytes)
{
   os_int
        ix;

	/* If allocated bytes is larger or equal than maximum block size, this goes automatically
	   to last group.
	 */
	if (bytes >= osal_global->memstate.max_block_sz)
	{
		return osal_global->memstate.n - 1;
	}

	ix = osal_memory_get_block_ix(bytes);

	if (bytes < osal_global->memstate.block_sz[ix])
	{
#if OSAL_MEMORY_DEBUG
		if (ix-- == 0)
		{
			osal_debug_error("Programming error");
		}
#else
		ix--;
#endif
	}

	return ix;
}


/**
****************************************************************************************************

  @brief Allocate a chunk of memory from operating system.

  The osal_malloc_chunk() allocates a chunk of memory from operating system. 
  
  If OSAL_PROCESS_CLEANUP_SUPPORT define is nonzero, part of chunk is used to maintain linked list
  of chunks, so that all memory can be released once the process exits or OSAL is restarted.

  @param   request_bytes Minimum number of bytes to allocate for use.

  @return  If the function succeeds to allocate the chunk of memory, it returns OSAL_SUCCESS (0).
		   Other values indicate failure, exspecially OSAL_STATUS_MEMORY_ALLOCATION_FAILED 
		   indicates that operating system memory allocation failed.
           See @ref osalStatus "OSAL function return codes" for full list.

****************************************************************************************************
*/
static osalStatus osal_allocate_chunk(
    os_memsz request_bytes)
{
    os_memsz
        allocated_bytes;

    os_char
        *chunk;

    os_short
        ix;

    osalMemorySliceHeader
        *slice;

#if OSAL_PROCESS_CLEANUP_SUPPORT
    osalMemoryChunkHeader
        *chunk_hdr;

    /* If we need cleanup code, add size of osalMemoryChunkHeader structure to request size.
     */
    request_bytes += sizeof(osalMemoryChunkHeader);
#endif

    /* Allocate chunk of memory from operating system. If this fails, return an error code.
     */
    chunk = osal_global->sysmem_alloc_func(request_bytes, &allocated_bytes);
    if (chunk == OS_NULL) return OSAL_STATUS_MEMORY_ALLOCATION_FAILED;

    /* Update resource monitor with amount of memory allocated from operating system
     */
#if OSAL_RESOURCE_MONITOR
    osal_resource_monitor_update(OSAL_RMON_SYSTEM_MEMORY_ALLOCATION, allocated_bytes);
#endif

#if OSAL_PROCESS_CLEANUP_SUPPORT
    /* Again if we need cleanup code, set up osalMemoryChunkHeader header structure and join
       the new chunk to list of allocated chunks. This is used only for memory cleanup
       osal_memory_shutdown() function.
     */
    chunk_hdr = (osalMemoryChunkHeader*)chunk;
    chunk_hdr->next_chunk = osal_global->memstate.chunk_list;
    osal_global->memstate.chunk_list = chunk_hdr;

    /* Forget about the header.
     */
    chunk += sizeof(osalMemoryChunkHeader);
    allocated_bytes -= sizeof(osalMemoryChunkHeader);
#endif

    /* Get index by allocated size.
     */
    ix = osal_memory_get_slice_ix(allocated_bytes);

    /* Join to chain of free chunks.
     */
    slice = (osalMemorySliceHeader*)chunk;
    slice->next_slice = osal_global->memstate.first_slice[ix];
    slice->bytes_left = allocated_bytes;
    osal_global->memstate.first_slice[ix] = slice;

    /* Success.
     */
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Allocate a slice of memory from memory chunk.

  The osal_memory_allocate_slice() allocates a slice of chunk allocated from operating system.
  If there are no allocated chunks from which the slice can be cut out, the function allocates
  a new chunk.

  @param   ix Index of memory block size to allocate. Index refers to block_sz array, which
		   contains memory block

  @return  Pointer to memory block of size specified by ix. If operating system memory 
		   allocation is called and it fails, the function returns OS_NULL.

****************************************************************************************************
*/
static void *osal_memory_allocate_slice(
    os_int ix)
{
   os_memsz
        bytes_left,
        chunk_bytes,
        bytes;

   os_int
        n,
        i,
        j;

   os_memsz
        sz0;

   os_char
        *memory_block,
        *p;

    osalMemorySliceHeader
        **first_slice,
        *slice;

    /* Get pointers to block size by chain index array and number of items in it.
     */
    first_slice = osal_global->memstate.first_slice;
    n = osal_global->memstate.n;
    bytes = osal_global->memstate.block_sz[ix];

    /* Try twice. First time we just try to get slice of existing chunk. If none is
	   available, new chunk is allocated and slice is taken from that.
     */
    for (j = 0; j < 2; j++)
    {
        for (i = ix;
             i < n;
             i++)
        {
            slice = first_slice[i];
            if (slice)
            {
                /* Remove the slice from list of slices of this size and reserve
                   space for the memory block.
                 */
                first_slice[i] = slice->next_slice;
                bytes_left = slice->bytes_left - bytes;
                memory_block = (os_char*)slice;
                sz0 = osal_global->memstate.block_sz[0];

				/* If there is so little left of this chunk, that there is
				   no space for the slicing header, just split left overs
				   as free smallest available memory blocks.
				 */
                if (bytes_left < sizeof(osalMemorySliceHeader) ||
					bytes_left < sz0)
                {

                    p = memory_block + bytes;

                    while (bytes_left >= sz0)
                    {
                        *((void**)p) = osal_global->memstate.first_free_block[0];
                        osal_global->memstate.first_free_block[0] = p;

                        p += sz0;
                        bytes_left -= sz0;
                    }
                }

				/* Generate new slice header and map again.
				 */
                else
                {
					/* Place the new slice header just after memory block.
					 */
                    slice = (osalMemorySliceHeader*)(memory_block + bytes);

                    /* Get index where to join by bytes left in chunk.
                     */
                    i = osal_memory_get_slice_ix(bytes_left);

                    /* Join to new position.
                     */
                    slice->next_slice = first_slice[i];
                    slice->bytes_left = bytes_left;
                    first_slice[i] = slice;
                }

				/* Return pointer to memory block.
				 */
                return memory_block;
            }
        }

        if (bytes <= OSAL_MEMORY_CHUNK_SIZE/3)
        {
            chunk_bytes = (OSAL_MEMORY_CHUNK_SIZE + bytes - 1) / bytes;
            chunk_bytes *= bytes;
        }
        else
        {
            chunk_bytes = bytes;
        }

        if (osal_allocate_chunk(chunk_bytes))
            return OS_NULL;
    }

    return OS_NULL;
}
#endif


#if OSAL_DYNAMIC_MEMORY_ALLOCATION==0

/* List of prepared free static memory blocks.
 */
osalStaticMemBlock *osal_static_mem_block_list = OS_NULL;


/**
****************************************************************************************************

  @brief Add static memory block to be allocated by osal_memory_allocate_static_block (os_malloc macro).
  @anchor osal_memory_add_static_block

  The osal_memory_add_static_block() function adds a static memory block to list
  of prepared free static memoty blocks. This is used to emulate dynamic memory allocation
  using explicitely (deterministic) allocated static memory.

  @param   block Pointer to static memory block.
  @param   block_sz Used static block size in bytes.
  @param   block_alloc Allocated static block size in bytes. This is normally the same as block_sz.
           This is intended for one problem: Assume we need to add static block of two bytes.
           This is less than size of osalStaticMemBlock structure, which is needed to link the
           static blocks. So we allocate static block of minimum sizeof(osalStaticMemBlock) and
           this size as block_alloc and the two bytes we actually need as block_sz.
           Set 0 to check block_sz instead.
  @return  None.

****************************************************************************************************
*/
void osal_memory_add_static_block(
    void *block,
    os_int block_sz,
    os_int block_alloc)
{
    osalStaticMemBlock
        *b;

    osal_debug_assert(block_alloc ? block_alloc : block_sz >= sizeof(osalStaticMemBlock));

    b = (osalStaticMemBlock*)block;
    os_lock();
    b->next = osal_static_mem_block_list;
    osal_static_mem_block_list = b;
    b->block_sz = block_sz;
    os_unlock();
}

/**
****************************************************************************************************

  @brief Reserve a prepared static memory block.
  @anchor osal_memory_allocate_static_block

  The osal_memory_allocate_static_block() reserved a memory block with precisely matching size
  from list of free static memory blocks.

  @param   request_bytes Size of the memory block to reserve.
  @param   allocated_bytes Pointer to long integer into which to store the actual size of the
           allocated block (in bytes), always set to request_bytes. This argument can be OS_NULL
           if not needed.

  @return  Pointer to the memory block, or OS_NULL if the function failed (no static memory
           block with given size was found).

****************************************************************************************************
*/
os_char *osal_memory_allocate_static_block(
    os_memsz request_bytes,
    os_memsz *allocated_bytes)
{
    osalStaticMemBlock
        *b,
        *prev_b;

    if (allocated_bytes)
    {
        *allocated_bytes = request_bytes;
    }

    if (request_bytes <= 0) return OS_NULL;

    os_lock();
    prev_b = OS_NULL;
    for (b = osal_static_mem_block_list;
         b;
         b = b->next)
    {
        if (b->block_sz == request_bytes)
        {
            if (prev_b) prev_b->next = b->next;
            else osal_static_mem_block_list = b->next;
            os_unlock();
            return (os_char*)b;
        }

        prev_b = b;
    }
    os_unlock();
    osal_debug_error("Failed to reserve static memory block\n");
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Release a static memory block.
  @anchor osal_memory_free_static_block

  The osal_memory_free_static_block() function placed a block of memory reserved
  by osal_memory_allocate_static_block() function back to list of free static memory blocks.

  @param   memory_block Pointer to memory block to release. If this pointer is OS_NULL, then
           the function does nothing.
  @param   bytes Either requested or allocated size of memory block.

  @return  None.

****************************************************************************************************
*/
void osal_memory_free_static_block(
    void *memory_block,
    os_memsz bytes)
{
    osalStaticMemBlock
        *b;

    if (memory_block == OS_NULL) return;

    b = (osalStaticMemBlock*)memory_block;
    os_lock();
    b->next = osal_static_mem_block_list;
    osal_static_mem_block_list = b;
    b->block_sz = bytes;
    os_unlock();
}


#endif
