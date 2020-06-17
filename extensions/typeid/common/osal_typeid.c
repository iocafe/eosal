/**

  @file    modules/typeid/osal_typeid.c
  @brief   Enumeration of data types and type name - type id conversions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  This is typeid module header file. This module enumerates data types and implements functions
  for converting type name (text) to type identifier (integer) and vice versa, plus function
  to get type size in bytes. To use type enumeration only, just include this header file.
  If also functions are needed, link with the typeid library.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_TYPEID_SUPPORT

/** Structure to store information about data type (includes type name).
 */
typedef struct
{
    /** Data type name string.
     */
    const os_char *name;

    /* Data type size in bytes. 0 if variable or unknown.
     */
    os_int sz;
}
osalTypeInfo;

/** Type names and sizes.
 */
static OS_FLASH_MEM osalTypeInfo osal_typeinfo[] = {
    {"undef", 0},					  /* OS_UNDEFINED_TYPE = 0 */
    {"boolean", sizeof(os_boolean)},  /* OS_BOOLEAN = 1 */
    {"char", sizeof(os_char)},		  /* OS_CHAR = 2 */
    {"uchar", sizeof(os_uchar)},	  /* OS_UCHAR = 3 */
    {"short", sizeof(os_short)},	  /* OS_SHORT = 4 */
    {"ushort", sizeof(os_ushort)},	  /* OS_USHORT = 5 */
    {"int", sizeof(os_int)},		  /* OS_INT = 6 */
    {"uint", sizeof(os_uint)},		  /* OS_UINT = 7 */
    {"int64", sizeof(os_int64)},	  /* OS_INT64 = 8 */
    {"long", sizeof(os_long)},		  /* OS_LONG = 9 */
    {"float", sizeof(os_float)},	  /* OS_FLOAT = 10 */
    {"double", sizeof(os_double)},	  /* OS_DOUBLE = 11 */
    {"dec01", sizeof(os_short)},	  /* OS_DEC01 = 12 */
    {"dec001", sizeof(os_short)},	  /* OS_DEC001 = 13 */
    {"str", 0},	                      /* OS_STR = 14 */
    {"object", 0},	                  /* OS_OBJECT = 15 */
    {"pointer", sizeof(os_pointer)}}; /* OS_POINTER = 16 */


/** Number of rows in osal_typeinfo table.
 */
#define OSAL_NRO_TYPE_INFO_ROWS (sizeof(osal_typeinfo) / sizeof(osalTypeInfo))

#else

/** Type size.
 */
static OS_FLASH_MEM os_char osal_typesz[] = {
    0,                                /* OS_UNDEFINED_TYPE = 0 */
    sizeof(os_boolean),               /* OS_BOOLEAN = 1 */
    sizeof(os_char),		          /* OS_CHAR = 2 */
    sizeof(os_uchar),                 /* OS_UCHAR = 3 */
    sizeof(os_short),                 /* OS_SHORT = 4 */
    sizeof(os_ushort),                /* OS_USHORT = 5 */
    sizeof(os_int),                   /* OS_INT = 6 */
    sizeof(os_uint),                  /* OS_UINT = 7 */
    sizeof(os_int64),                 /* OS_INT64 = 8 */
    sizeof(os_long),                  /* OS_LONG = 9 */
    sizeof(os_float),                 /* OS_FLOAT = 10 */
    sizeof(os_double),                /* OS_DOUBLE = 11 */
    sizeof(os_short),                 /* OS_DEC01 = 12 */
    sizeof(os_short),                 /* OS_DEC001 = 13 */
    0,                                /* OS_STR = 14 */
    0,                                /* OS_OBJECT = 15 */
    sizeof(os_pointer)};              /* OS_POINTER = 16 */

/** Number of rows in osal_typeinfo table.
 */
#define OSAL_NRO_TYPE_INFO_ROWS (sizeof(osal_typesz) / sizeof(os_char))

#endif


#if OSAL_TYPEID_SUPPORT
/**
****************************************************************************************************

  @brief Convert type name string to type identifier (integer).

  The osal_typeid_from_name function converts type name like "int" or "double" to type identifier
  integer.

  @param   name Type name to convert.
  @return  Type identifier matching to name. The function returns OS_UNDEFINED_TYPE (0) if
           name given as argument doesn't match to any type name.

****************************************************************************************************
*/
osalTypeId osal_typeid_from_name(
    const os_char *name)
{
    os_int
        i;

    os_char
        name0;

    const osalTypeInfo
        *tinfo;

    name0 = name[0];
    tinfo = osal_typeinfo;

    for (i = 0; i < OSAL_NRO_TYPE_INFO_ROWS; i++)
    {
        if (*tinfo->name == name0)
        {
            if (!os_strcmp(name, tinfo->name))
            {
                return (osalTypeId)i;
            }
        }
        tinfo++;
    }

    return OS_UNDEFINED_TYPE;
}


/**
****************************************************************************************************

  @brief Convert type identifier to type name string.

  The osal_typeid_to_name function converts type identifier like OS_INT (5) or OS_DOUBLE (10)
  to type name like "int" or "double".

  @param   type_id Type identifier.
  @return  Type name corresponding to type identifier given as argument. Enupt string if type
           identifier is illegal.

****************************************************************************************************
*/
const os_char *osal_typeid_to_name(
    osalTypeId type_id)
{
    if ((os_int)type_id < 0 || (os_int)type_id >= OSAL_NRO_TYPE_INFO_ROWS)
    {
        return osal_str_empty;
    }

    return osal_typeinfo[type_id].name;
}


/**
****************************************************************************************************

  @brief Get type size in bytes (code with type names).

  The osal_type_size function gets data type size in bytes.

  @param   type_id Type identifier.
  @return  Size in bytes, 0 if variable or unknown.

****************************************************************************************************
*/
os_memsz osal_type_size(
    osalTypeId type_id)
{
    if ((os_int)type_id < 0 || (os_int)type_id >= OSAL_NRO_TYPE_INFO_ROWS)
    {
        return 0;
    }

    return osal_typeinfo[type_id].sz;
}

#else

/**
****************************************************************************************************

  @brief Get type size in bytes (code without type names).

  The osal_type_size function gets data type size in bytes.

  @param   type_id Type identifier.
  @return  Size in bytes, 0 if variable or unknown.

****************************************************************************************************
*/
os_memsz osal_type_size(
    osalTypeId type_id)
{
    if ((os_int)type_id < 0 || (os_int)type_id >= OSAL_NRO_TYPE_INFO_ROWS)
    {
        return 0;
    }

    return osal_typesz[type_id];
}

#endif
