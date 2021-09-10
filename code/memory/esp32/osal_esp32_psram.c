/**

  @file    memory/esp32/osal_esp32_psram.c
  @brief   Operating system memory allocation.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  In addition to RAM on chip, the ESP32 can use pseudo static RAM trough SPI bus.
  The PSRAM is large, but much slower than on chip RAM. The esp-cam development boards
  and some other development boards have PSRAM installed.

  Use "-D BOARD_HAS_PSRAM" in platformio.ini to compile with PSRAM support.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosal.h"
#ifdef OSAL_ESP32
#if OSAL_PSRAM_SUPPORT

/**
****************************************************************************************************

  @brief Allocate a memory block from PSRAM.
  @anchor osal_psram_alloc

  The osal_psram_alloc() allocated a memory block from heap in PSRAM (pseudo static RAM).
  The PSRAM is typically large, but much slower than on chip RAM.

  @param   request_bytes The function allocates at least the amount of memory requested by this
           argument.

  @param   allocated_bytes Pointer to long integer into which to store the actual size of the
           allocated block (in bytes). The actual size is greater or equal to requested size.
           If actual size is not needed, this parameter can be set to OS_NULL.

  @return  Pointer to the allocated memory block, or OS_NULL if the function failed (out of
           memory).

****************************************************************************************************
*/
os_char *osal_psram_alloc(
    os_memsz request_bytes,
    os_memsz *allocated_bytes)
{
    os_char *mem;

    if (allocated_bytes) {
        *allocated_bytes = request_bytes;
    }

    mem = heap_caps_malloc(request_bytes, MALLOC_CAP_SPIRAM);
    if (mem == NULL) {
        osal_debug_error("osal_psram_alloc failed");
        osal_error(OSAL_SYSTEM_ERROR, eosal_mod, OSAL_STATUS_MEMORY_ALLOCATION_FAILED, OS_NULL);
        return NULL;
    }

    osal_resource_monitor_update(OSAL_RMON_SYSTEM_MEMORY_ALLOCATION, request_bytes);
    return mem;
}


/**
****************************************************************************************************

  @brief Release a memory block allocated from PSRAM.
  @anchor osal_sysmem_free

  The osal_psram_free() function releases a block of memory allocated from operating system
  by osal_psram_free() function.

  @param   memory_block Pointer to memory block to release. If this pointer is OS_NULL, then
           the function does nothing.
  @param   bytes Not needed here, but allocated block size must be still given to enable
           fallback to os_malloc/os_free when PSRAM is not available.

****************************************************************************************************
*/
void osal_psram_free(
    void *memory_block,
    os_memsz bytes)
{
    if (memory_block) {
        free(memory_block);
        osal_resource_monitor_update(OSAL_RMON_SYSTEM_MEMORY_ALLOCATION, -bytes);
    }
}

#endif
#endif
