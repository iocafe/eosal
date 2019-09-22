/**

  @file    persistent/arduino/osal_arduino_persistent.c
  @brief   Save persistent parameters on Arduino EEPROM.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    21.9.2019

  Arduino EEPROM api is used because it is well standardized. Hardware underneath can be flash,
  in case EEPROM omulation.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_PERSISTENT_SUPPORT
#include <Arduino.h>
#include <EEPROM.h>



typedef struct
{
    os_ushort pos, sz;
    os_ushort checksum;
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

/* Some random number to mark initialized header.
 */
#define MY_HEADER_INITIALIZED 0xB3


static myEEPROMHeader hdr;
static os_ushort eeprom_sz;
static os_boolean os_persistent_lib_initialized = OS_FALSE;

/* Forward referred static functions.
 */
static osalStatus os_persistent_delete_block(
    osPersistentBlockNr block_nr);

static void os_persistent_read(
    os_uchar *buf,
    os_ushort addr,
    os_ushort n);

static void os_persistent_write(
    const os_uchar *buf,
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

    eeprom_sz = EEPROM.Length();

    /* Read header.
     */
    if (read(&hdr, HDR_ADDR, sizeof(hdr))) goto getout;

    /* Validate header checksum and that header is initialized.
     */
    checksum = os_checksum(hdr.blk, OS_N_PBNR * sizeof(myEEPROMBlock));
    if (checksum == hdr.checksum && hdr.initialized == MY_HEADER_INITIALIZED) return;

getout:
    os_memclear(&hdr, sizeof(hdr));
    os_persistent_lib_initialized = OS_TRUE;
}


/**
****************************************************************************************************

  @brief Load parameter block (usually structure) from persistent storage.
  @anchor os_persistent_load

  The os_persistent_load() function loads parameter structure identified by block number
  from the persistant storage. Load all parameters when micro controller starts, not during
  normal operation. If data cannot be loaded, the function leaves the block as is.

  @param   block_nr Parameter block number, see osal_persistent.h.
  @param   block Pointer to block (structure) to load.
  @param   block_sz Block size in bytes.
  @return  Number of bytes read. Zero if failed. Returned value maxes at block_sz.

****************************************************************************************************
*/
os_memsz os_persistent_load(
    osPersistentBlockNr block_nr,
    os_uchar *block,
    os_memsz block_sz)
{
    os_uchar *tmp;
    os_ushort saved_pos, saved_sz;

    if (!os_persistent_lib_initialized)
    {
        os_persistent_initialze(OS_NULL);
    }

    if (block_nr < 0 || block_nr >= OS_N_PBNR || hdr.initialized != MY_HEADER_INITIALIZED) return 0;

    saved_pos = hdr.blk[block_nr].pos;
    saved_sz = hdr.blk[block_nr].len;
    if (saved_pos < sizeof(hdr) || saved_pos + (os_uint)saved_sz > eeprom_sz
        || saved_sz == 0 || block_sz <= 0)
    {
        return 0;
    }
    if (block_sz > saved_sz) block_sz = saved_sz;

    tmp = os_malloc(saved_sz, OS_NULL);
    if (tmp == OS_NULL) return 0;
    os_persistent_read(tmp, saved_pos, saved_sz);

    if (checksum(tmp, block_sz) != block[block_nr].checksum)
    {
        block_sz = 0;
        goto getout;
    }

    os_memcpy(block, tmp, block_sz);

getout:
    os_free(tmp, saved_sz);
    return block_sz;
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
  @param   commit If OS_TRUE, parameters are immediately committed to persistant storage.
  @return  OSAL_SUCCESS indicates all fine, other return values indicate on error.

****************************************************************************************************
*/
osalStatus os_persistent_save(
    osPersistentBlockNr block_nr,
    os_uchar *block,
    os_memsz block_sz,
    os_boolean commit)
{
    os_ushort saved_pos, saved_sz;
    osalStatus s = OSAL_SUCCESS;
    int i;

    if (!os_persistent_lib_initialized)
    {
        os_persistent_initialze(OS_NULL);
    }

    if (block_nr < 0 || block_nr >= OS_N_PBNR) return 0;

    /* If the block has gotten larger, delete old one.
     */
    if (block_sz > hdr.blk[block_nr].sz)
    {
        if (os_persistent_delete_block(block_nr))
        {
            os_memclear(&hdr, sizeof(hdr));
            goto getout;
        }
    }

    /* If we need to allocate space for the block.
     */
    if (hdr.blk[block_nr].sz == 0)
    {
        first_free = sizeof(hdr);
        for (i = 0; i< OS_N_PBNR; i++)
        {
            pos = hdr.blk[block_nr].pos + hdr.blk[block_nr].len;
            if (pos > first_free) first_free = pos;
        }

        hdr.blk[block_nr].pos = block_sz;
        hdr.blk[block_nr].sz = first_free;
        touched = OS_TRUE;

        if (first_free + block_sz > eeprom_sz)
        {
            os_memclear(&hdr, sizeof(hdr));
            goto getout;
        }
    }

    os_persistent_write(block, hdr.blk[block_nr].pos, block_sz);

getout:
    if (commit)
    {
        os_persistent_commit();
    }

    return s;
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
osalStatus os_persistent_commit(
    void)
{
    if (!hdr.touched || !os_persistent_lib_initialized) return;
    hdr.checksum = os_checksum(hdr.blk, OS_N_PBNR * sizeof(myEEPROMBlock));
    hdr.initialized = MY_HEADER_INITIALIZED;
    hdr.touched = OS_FALSE;

    return write(&hdr, sizeof(hdr));
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
    os_ushort saved_pos, saved_sz;
    os_ushort bpos[OS_N_PBNR], blen[OS_N_PBNR], bnr[OS_N_PBNR];
    int i, j, n;

    saved_pos = hdr.blk[block_nr].pos;
    saved_sz = hdr.blk[block_nr].len;

    if (saved_pos < sizeof(hdr) || saved_pos + (os_uint)saved_sz > eeprom_sz
        || saved_sz == 0 || block_sz <= 0)
    {
        return saved_pos ? OSAL_STATUS_FAILED : OSAL_SUCCESS;
    }

    /* Find out blocks in bigger addresses.
     */
    last_block_nr = 0;
    first_free = 0;
    n = 0;

    for (i = 0; i<OS_N_PBNR; i++)
    {
        sz = blk[i].sz;
        if (sz == 0) continue;
        pos = blk[i].pos;
        if (pos < sizeof(hdr) || pos + (os_uint)sz > eeprom_sz || sz == 0)
        {
            return OSAL_STATUS_FAILED;
        }

        if (pos <= saved_pos) continue;

        bpos[n] = pos;
        bsz[n] = sz;
        bnr[i] = i;
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
        os_persistent_move(bpos[i] - saved_pos, bpos[i], bsz[i]);
        hdr.blk[bnr[i]].pos -= saved_pos;
    }

    /* Clear data on deleted block and mark header touched.
     */
    os_memclear(hdr.blk + block_nr, sizeof(myEEPROMBlock));
    hdr.touched = 1;
}


/**
****************************************************************************************************

  @brief Read data from EEPROM.
  @anchor os_persistent_read

  The os_persistent_read() function reads n bytes from EEPROM starting from addr into buf.

  @param   buf Buffer where to store the data.
  @param   addr First address to read.
  @param   n Number of bytes to read.
  @return  None.

****************************************************************************************************
*/
static void os_persistent_read(
    os_uchar *buf,
    os_ushort addr,
    os_ushort n)
{
    while (n--)
    {
        *(buf++) = EEPROM.read(addr++);
    }
}


/**
****************************************************************************************************

  @brief Write data to EEPROM.
  @anchor os_persistent_delete_block

  The os_persistent_write() function writes n bytes from buf to EEPROM starting from addr.

  @param   buf Buffer from where to write the data.
  @param   addr First address to read.
  @param   n Number of bytes to read.
  @return  None.

****************************************************************************************************
*/
static void os_persistent_write(
    const os_uchar *buf,
    os_ushort addr,
    os_ushort n)
{
    while (n--)
    {
       EEPROM.write(addr++, *(buf++));
    }
}


/**
****************************************************************************************************

  @brief Move data in EEPROM to compress after deleting a block.
  @anchor os_persistent_delete_block

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

    while (n--)
    {
        c = EEPROM.read(srcaddr++);
       EEPROM.write(dstaddr++, c);
    }
}


#endif
