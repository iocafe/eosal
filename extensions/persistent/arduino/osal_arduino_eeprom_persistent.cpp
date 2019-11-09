/**

  @file    persistent/arduino/osal_arduino_eeprom_persistent.c
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
    /* Block address in EEPROM
     */
    os_ushort pos;

    /* Number of bytes reserved for the block.
     */
    os_ushort sz;

    /* Number of bytes currently used.
     */
    os_ushort data_sz;

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

#define MY_EEPROM_MIN_SIZE 1024

static myEEPROMHeader hdr;
static os_ushort eeprom_sz;
static os_boolean os_persistent_lib_initialized = OS_FALSE;

/* Forward referred static functions.
 */
static osalStatus os_persistent_delete_block(
    osPersistentBlockNr block_nr);

static void os_persistent_read(
    os_char *buf,
    os_ushort addr,
    os_ushort n);

static void os_persistent_write(
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

    EEPROM.begin(min_eeprom_sz);

    eeprom_sz = EEPROM.length();
    if (eeprom_sz == 0)
    {
        osal_console_write("EEPROM length 0 reported, using default: ");
        eeprom_sz = min_eeprom_sz;
    }
    osal_console_write("EEPROM size = ");
    osal_int_to_string(buf, sizeof(buf), eeprom_sz);
    osal_console_write(buf);
    osal_console_write("\n");

    /* Read header.
     */
    os_persistent_read((os_char*)&hdr, 0, sizeof(hdr));

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
    void *block,
    os_memsz block_sz)
{
    os_ushort saved_pos, data_sz;

    if (!os_persistent_lib_initialized)
    {
        os_persistent_initialze(OS_NULL);
    }

    if (block_nr < 0 || block_nr >= OS_N_PBNR || hdr.initialized != MY_HEADER_INITIALIZED) return 0;

    saved_pos = hdr.blk[block_nr].pos;
    data_sz = hdr.blk[block_nr].data_sz;
    if (saved_pos < sizeof(hdr) || saved_pos + (os_uint)data_sz > eeprom_sz
        || data_sz == 0 || block_sz <= 0)
    {
        return 0;
    }
    if (block_sz > data_sz) block_sz = data_sz;

    /* tmp = (os_uchar*)os_malloc(data_sz, OS_NULL); */
    os_char tmp[data_sz];
    os_persistent_read(tmp, saved_pos, data_sz);

    if (os_checksum(tmp, data_sz, OS_NULL) != hdr.blk[block_nr].checksum)
    {
        block_sz = 0;
        goto getout;
    }

    os_memcpy(block, tmp, block_sz);

getout:
    /* os_free(tmp, data_sz); */
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
    const void *block,
    os_memsz block_sz,
    os_boolean commit)
{
    os_ushort first_free, pos;
    osalStatus s = OSAL_SUCCESS;
    int i;

    if (!os_persistent_lib_initialized)
    {
        os_persistent_initialze(OS_NULL);
    }

    if (block_nr < 0 || block_nr >= (os_memsz)OS_N_PBNR) return OSAL_STATUS_FAILED;

    hdr.touched = OS_TRUE;

    /* If the block has gotten larger, delete old one.
     */
    if (block_sz > hdr.blk[block_nr].sz)
    {
        s = os_persistent_delete_block(block_nr);
        if (s)
        {
            os_memclear(&hdr, sizeof(hdr));
            goto getout;
        }
    }

    /* If we need to allocate space for the block.
     */
    if (hdr.blk[block_nr].sz == 0)
    {
        first_free = (os_ushort)sizeof(hdr);
        for (i = 0; i< OS_N_PBNR; i++)
        {
            pos = hdr.blk[i].pos + hdr.blk[i].sz;
            if (pos > first_free) first_free = pos;
        }

        hdr.blk[block_nr].pos = first_free;
        hdr.blk[block_nr].sz = block_sz;

        if (first_free + block_sz > eeprom_sz)
        {
            os_memclear(&hdr, sizeof(hdr));
            goto getout;
        }
    }

    hdr.blk[block_nr].data_sz = block_sz;
    hdr.blk[block_nr].checksum = os_checksum((os_char*)block, block_sz, OS_NULL);

    os_persistent_write((os_char*)block, hdr.blk[block_nr].pos, block_sz);

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
    if (!hdr.touched || !os_persistent_lib_initialized) return OSAL_SUCCESS;
    hdr.checksum = os_checksum((os_char*)hdr.blk, OS_N_PBNR * sizeof(myEEPROMBlock), OS_NULL);
    hdr.initialized = MY_HEADER_INITIALIZED;
    hdr.touched = OS_FALSE;

    os_persistent_write((os_char*)&hdr, 0, sizeof(hdr));

    EEPROM.commit();
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
        os_persistent_move(bpos[i] - saved_pos, bpos[i], bsz[i]);
        hdr.blk[bnr[i]].pos -= saved_pos;
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
  @anchor os_persistent_read

  The os_persistent_read() function reads n bytes from EEPROM starting from addr into buf.

  @param   buf Buffer where to store the data.
  @param   addr First address to read.
  @param   n Number of bytes to read.
  @return  None.

****************************************************************************************************
*/
static void os_persistent_read(
    os_char *buf,
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
    const os_char *buf,
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
