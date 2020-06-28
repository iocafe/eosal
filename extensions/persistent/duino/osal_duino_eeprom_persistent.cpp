/**

  @file    persistent/duino/osal_arduino_eeprom_persistent.c
  @brief   Save persistent parameters on Arduino EEPROM.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Arduino EEPROM api is used because it is well standardized. Hardware underneath can be flash
  for EEPROM emulation.

  ONLY ONE BLOCK CAN BE OPEN AT THE TIME FOR WRITING.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_PERSISTENT_SUPPORT==OSAL_ARDUINO_EEPROM_API
#include <Arduino.h>
#include <EEPROM.h>


typedef struct
{
    os_ushort pos;      /* Block address in EEPROM.  */
    os_ushort sz;       /* Block size in bytes. */
    os_ushort read_ix;  /* Current read position */
    os_ushort checksum; /* Check sum. */
    os_int flags;       /* Operation, OSAL_PERSISTENT_WRITE or OSAL_PERSISTENT_WRITE bits */
}
myEEPROMBlock;


typedef struct
{
    myEEPROMBlock blk[OS_N_PBNR];
    os_ushort checksum;
    os_uchar initialized;
    os_uchar touched;
}
myEEPROMHeader;


/* An ID number to mark initialized header.
 */
#define MY_HEADER_INITIALIZED 0xB3

#define MY_EEPROM_MIN_SIZE 4096

static myEEPROMHeader hdr;
static os_uint eeprom_sz;
static os_boolean os_persistent_lib_initialized = OS_FALSE;

/* Forward referred static functions.
 */
static osalStatus os_persistent_commit(
    void);

static osalStatus os_persistent_delete_block(
    osPersistentBlockNr block_nr);

static void os_persistent_read_internal(
    os_char *buf,
    os_ushort addr,
    os_ushort n);

static void os_persistent_write_internal(
    const os_char *buf,
    os_ushort addr,
    os_ushort n);

static void os_persistent_move(
    os_ushort dstaddr,
    os_ushort srcaddr,
    os_ushort n);


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
    os_ushort checksum;
    os_char buf[OSAL_NBUF_SZ];
    os_memsz min_eeprom_sz = MY_EEPROM_MIN_SIZE;

    /* Get minimum eerom size from parameters.
     */
    if (prm) if (prm->min_eeprom_sz)
    {
        min_eeprom_sz = prm->min_eeprom_sz;
    }

    /* Do not initialized again by load or save call.
     */
    os_persistent_lib_initialized = OS_TRUE;

    if (!EEPROM.begin(min_eeprom_sz))
    {
         osal_debug_error("failed to initialise EEPROM");
    }

    eeprom_sz = EEPROM.length();
    if (eeprom_sz == 0)
    {
        osal_console_write("EEPROM length 0 reported, using default: ");
        eeprom_sz = min_eeprom_sz;
    }
    osal_console_write("EEPROM size = ");
    osal_int_to_str(buf, sizeof(buf), eeprom_sz);
    osal_console_write(buf);
    osal_console_write("\n");

    /* Read header.
     */
    os_persistent_read_internal((os_char*)&hdr, 0, sizeof(hdr));

    /* Validate header checksum and that header is initialized.
     */
    checksum = os_checksum((os_char*)hdr.blk, OS_N_PBNR * sizeof(myEEPROMBlock), OS_NULL);
    if (checksum == hdr.checksum && hdr.initialized == MY_HEADER_INITIALIZED) return;

    os_memclear(&hdr, sizeof(hdr));
}


/**
****************************************************************************************************

  @brief Release any resources allocated for the persistent storage.
  @anchor os_persistent_shutdown

  The os_persistent_shutdown() function is just place holder for future implementations.
  @return  None.

****************************************************************************************************
*/
void os_persistent_shutdown(
    void)
{
}


/**
****************************************************************************************************

  @brief Get pointer to persistant data block within persistent storage.
  @anchor os_persistent_get_ptr

  If persistant storage is in micro-controller's flash, we can just get pointer to data block
  and data size. If persistant storage is on such media (file system, etc) that direct
  pointer is not available, the function returns OSAL_STATUS_NOT_SUPPORTED and function
  os_persistent_load() must be called to read the data to local RAM.

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   block Pointer to block (structure) to load.
  @param   block_sz Block size in bytes.
  @param   flags OSAL_PERSISTENT_SECRET flag enables accessing the secret. It must be only
           given in safe context.
  @return  OSAL_SUCCESS of successful. Value OSAL_STATUS_NOT_SUPPORTED indicates that
           pointer cannot be aquired on this platform and os_persistent_load() must be
           called instead. Other values indicate an error.

****************************************************************************************************
*/
osalStatus os_persistent_get_ptr(
    osPersistentBlockNr block_nr,
    const os_char **block,
    os_memsz *block_sz,
    os_int flags)
{
#if EOSAL_RELAX_SECURITY == 0
    /* Reading or writing seacred block requires secret flag. When this function
       is called for data transfer, there is no secure flag and thus secret block
       cannot be accessed to break security.
     */
    if ((block_nr == OS_PBNR_SECRET || block_nr == OS_PBNR_SERVER_KEY) &&
        (flags & OSAL_PERSISTENT_SECRET) == 0)
    {
        return OSAL_STATUS_NOT_AUTOHORIZED;
    }
#endif

    return OSAL_STATUS_NOT_SUPPORTED;
}


/**
****************************************************************************************************

  @brief Open persistent block for reading or writing.
  @anchor os_persistent_open

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   block_sz Pointer to integer where to store block size when reading the persistent block.
           This is intended to know memory size to allocate before reading.
  @param   flags OSAL_PERSISTENT_READ or OSAL_PERSISTENT_WRITE. Flag OSAL_PERSISTENT_SECRET
           enables reading or writing the secret, and must be only given in safe context.

  @return  Persistant storage block handle, or OS_NULL if the function failed.

****************************************************************************************************
*/
osPersistentHandle *os_persistent_open(
    osPersistentBlockNr block_nr,
    os_memsz *block_sz,
    os_int flags)
{
    myEEPROMBlock *block;
    os_ushort first_free, pos, cscalc, n, nnow, p, sz;
    os_char tmp[64];
    int i;

    /* This implementation doesn not support nor simulate flash programming.
     */
    if (block_nr == OS_PBNR_FLASH_PROGRAM)
    {
        return OS_NULL;
    }

#if EOSAL_RELAX_SECURITY == 0
    /* Reading or writing seacred block requires secret flag. When this function
       is called for data transfer, there is no secure flag and thus secret block
       cannot be accessed to break security.
     */
    if ((block_nr == OS_PBNR_SECRET || block_nr == OS_PBNR_SERVER_KEY) &&
        (flags & OSAL_PERSISTENT_SECRET) == 0)
    {
        return OS_NULL;
    }
#endif

    if (!os_persistent_lib_initialized)
    {
        os_persistent_initialze(OS_NULL);
    }

    block = hdr.blk + block_nr;
    block->flags = flags;

    if (flags & OSAL_PERSISTENT_WRITE)
    {
        first_free = (os_ushort)sizeof(hdr);
        for (i = 0; i < OS_N_PBNR; i++)
        {
            if (hdr.blk[i].sz == 0 || i == block_nr) continue;
            pos = hdr.blk[i].pos + hdr.blk[i].sz;
            if (pos > first_free) {
                first_free = pos;
            }
        }

        /* If this is not the last block, delete it.
         */
        if (first_free < block->pos && block->sz)
        {
            sz = block->sz; /* save size */
            if (os_persistent_delete_block(block_nr))
            {
                os_memclear(&hdr, sizeof(hdr));
                hdr.touched = OS_TRUE;
                os_persistent_commit();
                first_free = (os_ushort)sizeof(hdr);
            }
            else
            {
                first_free -= sz;
            }
        }
        block->pos = first_free;
        block->sz = 0;
        block->checksum = OSAL_CHECKSUM_INIT;
        hdr.touched = OS_TRUE;
    }
    else
    {
        if (block_sz) *block_sz = block->sz;
        if (block->sz == 0) return OS_NULL;
        block->read_ix = 0;

        /* Verify checksum
         */
        n = block->sz;
        p = block->pos;
        cscalc = OSAL_CHECKSUM_INIT;

        while (n > 0)
        {
            nnow = n;
            if (sizeof(tmp) < nnow) nnow = sizeof(tmp);
            os_persistent_read_internal(tmp, p, nnow);
            os_checksum(tmp, nnow, &cscalc);
            p += nnow;
            n -= nnow;
        }

        if (cscalc != block->checksum) return OS_NULL;
    }

    return (osPersistentHandle*)block;
}


/**
****************************************************************************************************

  @brief Close persistent storage block.
  @anchor os_persistent_close

  @param   handle Persistant storage handle. It is ok to call the function with NULL handle,
           just nothing happens.
  @param   flags OSAL_STREAM_DEFAULT (0) is all was written to persistant storage.
           OSAL_STREAM_INTERRUPT flag is set if transfer was interrupted.
  @return  None.

****************************************************************************************************
*/
void os_persistent_close(
    osPersistentHandle *handle,
    os_int flags)
{
    myEEPROMBlock *block;
    block = (myEEPROMBlock*)handle;

    if (block) if (block->flags & OSAL_PERSISTENT_WRITE)
    {
        os_persistent_commit();
    }
}


/**
****************************************************************************************************

  @brief Read data from persistent parameter block.
  @anchor os_persistent_read

  The os_persistent_read() function reads data from persistant storage block. Load all
  parameters when micro controller starts, not during normal operation.

  @param   handle Persistant storage handle.
  @param   block Pointer to buffer where to load persistent data.
  @param   buf_sz Number of bytes to read to buffer.
  @return  Number of bytes read. Can be less than buf_sz if end of persistent block data has
           been reached. 0 is fine if at end. -1 Indicates an error.

****************************************************************************************************
*/
os_memsz os_persistent_read(
    osPersistentHandle *handle,
    os_char *buf,
    os_memsz buf_sz)
{
    os_ushort n;
    myEEPROMBlock *block;
    block = (myEEPROMBlock*)handle;

    if (block) if (block->read_ix < block->sz)
    {
        n = block->sz - block->read_ix;
        if ((os_ushort)buf_sz < n) n = (os_ushort)buf_sz;

        os_persistent_read_internal(buf, block->pos + block->read_ix, n);
        block->read_ix += n;
        return n;
    }

    return -1;
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
osalStatus os_persistent_write(
    osPersistentHandle *handle,
    const os_char *buf,
    os_memsz buf_sz)
{
    myEEPROMBlock *block;
    block = (myEEPROMBlock*)handle;

    if (block)
    {
        osal_control_interrupts(OS_FALSE);
        os_persistent_write_internal(buf, block->pos + block->sz, buf_sz);
        os_checksum(buf, buf_sz, &block->checksum);
        block->sz += (os_ushort)buf_sz;
        hdr.touched = 1;
        osal_control_interrupts(OS_TRUE);
        return OSAL_SUCCESS;
    }

    return OSAL_STATUS_FAILED;
}



/**
****************************************************************************************************

  @brief Commit changes to persistent storage.
  @anchor os_persistent_commit

  The os_persistent_commit() function commit unsaved changed to persistent storage.

  @return  OSAL_SUCCESS indicates all fine, other return values indicate on corrupted
           EEPROM content.

****************************************************************************************************
*/
static osalStatus os_persistent_commit(
    void)
{
    if (!hdr.touched || !os_persistent_lib_initialized) return OSAL_SUCCESS;
    hdr.checksum = os_checksum((os_char*)hdr.blk, OS_N_PBNR * sizeof(myEEPROMBlock), OS_NULL);
    hdr.initialized = MY_HEADER_INITIALIZED;
    hdr.touched = OS_FALSE;

    osal_control_interrupts(OS_FALSE);
    os_persistent_write_internal((os_char*)&hdr, 0, sizeof(hdr));
    EEPROM.commit();

    osal_control_interrupts(OS_TRUE);
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Delete block from persistent storage.
  @anchor os_persistent_delete_block

  The os_persistent_delete_block() function gets called when memory block is expanded. In this case
  memory block being expanded is first deleted, and all consequent memory blocks are moved up.

  @return  OSAL_SUCCESS indicates all fine, other return values indicate on corrupted
           EEPROM content.

****************************************************************************************************
*/
static osalStatus os_persistent_delete_block(
    osPersistentBlockNr block_nr)
{
    os_ushort saved_pos, saved_sz, sz, pos, utmp;
    os_ushort bpos[OS_N_PBNR], bsz[OS_N_PBNR], bnr[OS_N_PBNR];
    int i, j, n;

    saved_pos = hdr.blk[block_nr].pos;
    saved_sz = hdr.blk[block_nr].sz;

    if (saved_pos < sizeof(hdr) || saved_pos + (os_uint)saved_sz > eeprom_sz
        || saved_sz == 0)
    {
        return saved_pos ? OSAL_STATUS_FAILED : OSAL_SUCCESS;
    }

    /* Find out blocks in bigger addresses.
     */
    n = 0;

    for (i = 0; i<OS_N_PBNR; i++)
    {
        sz = hdr.blk[i].sz;
        if (sz == 0) continue;
        pos = hdr.blk[i].pos;
        if (pos < sizeof(hdr) || pos + (os_uint)sz > eeprom_sz || sz == 0)
        {
            return OSAL_STATUS_FAILED;
        }

        if (pos <= saved_pos) continue;

        bpos[n] = pos;
        bsz[n] = sz;
        bnr[n] = i;
        n++;
    }

    /* Sort these
     */
    for (i = 0; i < n - 1; i++)
    {
        for (j = i + 1; j < n; j++)
        {
            if (bpos[i] > bpos[j])
            {
                utmp = bpos[i]; bpos[i] = bpos[j]; bpos[j] = utmp;
                utmp = bsz[i]; bsz[i] = bsz[j]; bsz[j] = utmp;
                utmp = bnr[i]; bnr[i] = bnr[j]; bnr[j] = utmp;
            }
        }
    }

    /* Move these on EEPROM
     */
    for (i = 0; i < n; i++)
    {
        os_persistent_move(bpos[i] - saved_sz, bpos[i], bsz[i]);
        hdr.blk[bnr[i]].pos -= saved_sz;
    }

    /* Clear data on deleted block and mark header touched.
     */
    os_memclear(hdr.blk + block_nr, sizeof(myEEPROMBlock));
    hdr.touched = 1;
    return OSAL_SUCCESS;
}


/**
****************************************************************************************************

  @brief Read data from EEPROM.
  @anchor os_persistent_read_internal

  The os_persistent_read_internal() function reads n bytes from EEPROM starting from addr into buf.

  @param   buf Buffer where to store the data.
  @param   addr First address to read.
  @param   n Number of bytes to read.
  @return  Number of bytes read. Can be less than buf_sz if end of persistent block data has
           been reached. 0 is fine if at end. -1 Indicates an error.

****************************************************************************************************
*/
static void os_persistent_read_internal(
    os_char *buf,
    os_ushort addr,
    os_ushort n)
{
    while (n--)
    {
        if (addr >= eeprom_sz)
        {
            osal_debug_error("READ Out of EEPROM space");
            break;
        }

        *(buf++) = EEPROM.read(addr++);
    }
}


/**
****************************************************************************************************

  @brief Write data to EEPROM.
  @anchor os_persistent_write_internal

  The os_persistent_write_internal() function writes n bytes from buf to EEPROM starting from addr.

  @param   buf Buffer from where to write the data.
  @param   addr First address to read.
  @param   n Number of bytes to read.
  @return  None.

****************************************************************************************************
*/
static void os_persistent_write_internal(
    const os_char *buf,
    os_ushort addr,
    os_ushort n)
{
    while (n--)
    {
        if (addr >= eeprom_sz)
        {
            osal_debug_error("WRITE Out of EEPROM space");
            break;
        }

        EEPROM.write(addr++, *(buf++));
    }
}


/**
****************************************************************************************************

  @brief Move data in EEPROM to compress after deleting a block.
  @anchor os_persistent_move

  The os_persistent_move() function...

  @param   dstaddr Destination address of first byte to move.
  @param   srcaddr Source address of first byte to move, srcaddr > dstaddr.
  @param   n Number of bytes to move.
  @return  None.

****************************************************************************************************
*/
static void os_persistent_move(
    os_ushort dstaddr,
    os_ushort srcaddr,
    os_ushort n)
{
    os_uchar c;

    osal_control_interrupts(OS_FALSE);

    while (n--)
    {
        c = EEPROM.read(srcaddr++);
        EEPROM.write(dstaddr++, c);
    }

    osal_control_interrupts(OS_TRUE);
}

#endif
