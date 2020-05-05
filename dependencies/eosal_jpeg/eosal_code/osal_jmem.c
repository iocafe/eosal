/**

  @file    eosal_jpeg/eosal_code/osal_jmem.c
  @brief   eosal API for libjpeg.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    5.5.2020

  This file provides a really simple implementation of the system dependent portion of
  the JPEG memory manager. This implementation  assumes that no backing-store files are
  needed: all required space can be obtained from os_malloc().

  This is very portable in the sense that it'll compile on almost anything, but you'd better
  have lots of main memory (or virtual memory) if you want to process big images.
  Note that the max_memory_to_use option is ignored by this implementation.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

  Copyright (C) 1991-1998, Thomas G. Lane. This file is derived from work of the Independent
  JPEG Group's software.

****************************************************************************************************
*/
#define JPEG_INTERNALS
#include "eosal_jpeg.h"
#include "code/jmemsys.h"		/* import the system-dependent declarations */


/*
 * Memory allocation and freeing are controlled by the regular library
 * routines malloc() and free().
 */

GLOBAL(void *)
jpeg_get_small (j_common_ptr cinfo, size_t sizeofobject)
{
  return (void *) os_malloc(sizeofobject, OS_NULL);
}

GLOBAL(void)
jpeg_free_small (j_common_ptr cinfo, void * object, size_t sizeofobject)
{
  os_free(object, sizeofobject);
}


/*
 * "Large" objects are treated the same as "small" ones.
 */

GLOBAL(void *)
jpeg_get_large (j_common_ptr cinfo, size_t sizeofobject)
{
  return (void *) os_malloc(sizeofobject, OS_NULL);
}

GLOBAL(void)
jpeg_free_large (j_common_ptr cinfo, void * object, size_t sizeofobject)
{
  os_free(object, sizeofobject);
}


/*
 * This routine computes the total memory space available for allocation.
 * Here we always say, "we got all you want bud!"
 */

GLOBAL(long)
jpeg_mem_available (j_common_ptr cinfo, long min_bytes_needed,
            long max_bytes_needed, long already_allocated)
{
  return max_bytes_needed;
}


/*
 * Backing store (temporary file) management.
 * Since jpeg_mem_available always promised the moon,
 * this should never be called and we can just error out.
 */

GLOBAL(void)
jpeg_open_backing_store (j_common_ptr cinfo, backing_store_ptr info,
             long total_bytes_needed)
{
  ERREXIT(cinfo, JERR_NO_BACKING_STORE);
}

/*
 * These routines take care of any system-dependent initialization and
 * cleanup required.  Here, there isn't any.
 */

GLOBAL(long)
jpeg_mem_init (j_common_ptr cinfo)
{
  return 0;			/* just set max_memory_to_use to 0 */
}

GLOBAL(void)
jpeg_mem_term (j_common_ptr cinfo)
{
  /* no work */
}
