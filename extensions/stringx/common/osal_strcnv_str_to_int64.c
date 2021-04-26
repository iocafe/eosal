/**

  @file    strcnv/common/osal_str_to_int64.c
  @brief   Convert string to 64 bit integer.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    26.4.2021

  Function osal_str_to_int64() to convert string to 64 bit integer.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_STRINGX_SUPPORT


/**
****************************************************************************************************

  @brief Convert a string to 64 bit integer.
  @anchor osal_str_to_int64

  The osal_str_to_int64() function converts a string to 64 bit integet value.

  If os_long type is 64 bits, this function just calls osal_str_to_int() to do the work.
  Otherwise real implementation is needed.

  @param   x Pointer to 64 bit integer into which to store the value. 
  @param   str Pointer to string to convert. 

  @return  Number of bytes parsed, 0 indicates an error. 

****************************************************************************************************
*/
os_memsz osal_str_to_int64(
    os_int64 *x, 
    os_char *str)
{
#if OSAL_LONG_IS_64_BITS
    os_memsz count;
	
	*x = (os_int64)osal_str_to_int(str, &count);
	return count;
#else
    os_boolean 
        negative, 
        ok;

    os_char 
        *p,
		*s;

    os_int64 
        v, 
		tmp,
		prev;

	/* If str argument is null pointer ?
	 */
    if (str == OS_NULL) 
    {
        osal_int64_set_long(x, 0);
        return 0;
    }

	/* Set initial values.
	 */
    osal_int64_set_long(&v, 0);
    p = str;
    negative = OS_FALSE;

    /* Skip white space characters.
     */
    while (osal_char_isspace(*p)) ++p;

    /* If value starts with '-' or '+' sign.
     */
    if (*p == '-' || *p == '+') 
    {
        if (*p == '-') negative = OS_TRUE;
        ++p;
        
        /* Skip white space characters.
		 */
        while (osal_char_isspace(*p)) ++p;
    }

	/* All "ok" for now, if we got at least one decimal digit.
	 */
    ok = (os_boolean)osal_char_isdigit(*p);

    /* Parse the value.
     */
	s = p;
    while (osal_char_isdigit(*p)) 
    {
		osal_int64_copy(&prev, &v);
        osal_int64_set_long(&tmp, (*(p++) - '0'));
        osal_int64_multiply(&v, &osal_int64_10);
        osal_int64_add(&v, &tmp);

        /* If there are 17 or more characters, check for overflow.
         */
        if (p - s >= 17) 
        {
            if (osal_int64_compare(&v, &prev) < 0) 
            {
                ok = OS_FALSE;
                break;
            }
        }
    }

	/* If value is negative, negate now.
	 */
    if (negative) osal_int64_negate(&v);

	/* Save the value.
	 */
    osal_int64_copy(x, &v);

	/* Return number of parsed characters, or 0 if the function failed.
	 */
    return ok ? (p - str) : 0;
#endif
}

#endif
