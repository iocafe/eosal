/**

  @file    modules/typeid/osal_typeid.c
  @brief   Enumeration of data types and type name - type id conversions.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    9.11.2011

  This is typeid module header file. This module enumerates data types and implements functions
  for converting type name (text) to type identifier (integer) and vice versa, plus function
  to get type size in bytes. To use type enumeration only, just include this header file. 
  If also functions are needed, link with the typeid library.

  Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_TYPEID_SUPPORT

/** Structure to store information about data type.
 */
typedef struct
{
    /** Data type name string.
     */
	os_char *name;

    /* Data type size in bytes. 0 if variable or unknown.
     */
	os_int sz;
}
osalTypeInfo;

/** Type names and sizes.
 */
static osalTypeInfo osal_typeinfo[] = {
    {"undef", 0},					  /* OS_UNDEFINED_TYPE = 0 */
	{"char", sizeof(os_char)},		  /* OS_CHAR = 1 */
	{"uchar", sizeof(os_uchar)},	  /* OS_UCHAR = 2 */
	{"short", sizeof(os_short)},	  /* OS_SHORT = 3 */
	{"ushort", sizeof(os_ushort)},	  /* OS_USHORT = 4 */
	{"int", sizeof(os_int)},		  /* OS_INT = 5 */
	{"uint", sizeof(os_uint)},		  /* OS_UINT = 6 */
	{"int64", sizeof(os_int64)},	  /* OS_INT64 = 7 */
	{"long", sizeof(os_long)},		  /* OS_LONG = 8 */
	{"float", sizeof(os_float)},	  /* OS_FLOAT = 9 */
	{"double", sizeof(os_double)},	  /* OS_DOUBLE = 10 */
	{"dec01", sizeof(os_short)},	  /* OS_DEC01 = 11 */
	{"dec001", sizeof(os_short)},	  /* OS_DEC001 = 12 */
	{"str", 0},	                      /* OS_STRING = 13 */
    {"object", 0},	                  /* OS_OBJECT = 14 */
    {"pointer", sizeof(os_pointer)}}; /* OS_POINTER = 15 */

/** Number of rows in osal_typeinfo table.
 */
#define OSAL_NRO_TYPE_INFO_ROWS (sizeof(osal_typeinfo) / sizeof(osalTypeInfo))


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
    os_char *name)
{
    os_int
        i;

    for (i = 0; i < OSAL_NRO_TYPE_INFO_ROWS; i++)
    {
        if (!os_strcmp(name, osal_typeinfo[i].name))
        {
            return (osalTypeId)i;
        }
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
os_char *osal_typeid_to_name(
    osalTypeId type_id)
{
    if (type_id < 0 || type_id >= OSAL_NRO_TYPE_INFO_ROWS)
    {
        return "";
    }

    return osal_typeinfo[type_id].name;
}


/**
****************************************************************************************************

  @brief Get type size in bytes.

  The osal_typeid_size function gets data type size in bytes.

  @param   type_id Type identifier. 
  @return  Size in bytes, 0 if variable or unknown.

****************************************************************************************************
*/
os_memsz osal_typeid_size(
    osalTypeId type_id)
{
    if (type_id < 0 || type_id >= OSAL_NRO_TYPE_INFO_ROWS)
    {
        return 0;
    }

    return osal_typeinfo[type_id].sz;
}

#endif
