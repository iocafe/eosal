/**

  @file    stringx/common/osal_str_get_next.c
  @brief   Find value of item in list string.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

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

  @brief Find beginning of next item or line in list string.
  @anchor osal_str_get_next

  The osal_str_get_next() function returns pointer to beginning of next item or next line
  in list string

  @param   list_str Pointer to list string to search. If the str is OS_NULL the function
           will return pointer to null character.

  @return  Pointer to first character on next line of list string. OS_NULL If none found.

****************************************************************************************************
*/
const os_char *osal_str_get_next(
    const os_char *list_str,
    os_short flags)
{
    const os_char *a, *b;
    os_char ac, bc;

    if (flags & OSAL_STR_NEXT_LINE)
    {
        ac = ';';
        bc = '\n';
    }
    else
    {
        ac = ',';
        bc = '\t';
    }

    /* If list string is NULL pointer, return pointer to null character.
     */
    if (list_str)
    {
        a = os_strchr((os_char*)list_str, ac);
        b = os_strchr((os_char*)list_str, bc);
        if (a)
        {
            if (b) if (b < a) a = b;
            return a+1;
        }

        if (b) return b + 1;
    }

    /* No next line.
     */
    return OS_NULL;
}


/**
****************************************************************************************************

  @brief Iterate trough items in string list.
  @anchor osal_str_list_iter

  The osal_str_list_iter() function is used to iterate trough list string. Before the first
  osal_str_copy_item call list_str_ptr is set to point the list string. The function copies one
  item from list into the buffer given as argument and advances list_str_ptr. When there are
  not more items in list, the function returns OSAL_STATUS_FAILED.

  @param   list_str Pointer to list string to search. If the str is OS_NULL the function
           will return pointer to null character.

  @return  OSAL_SUCCESS if successfull. OSAL_STATUS_FAILED if there are no more items in list.

****************************************************************************************************
*/
osalStatus osal_str_list_iter(
    os_char *buf,
    os_memsz buf_sz,
    const os_char **list_str_ptr,
    os_short flags)
{
    const os_char *next_ptr, *ptr;
    os_char *q;
    os_memsz n;

    ptr = *list_str_ptr;
    if (ptr == OS_NULL) return OSAL_STATUS_FAILED;
    next_ptr = osal_str_get_next(ptr, flags);

    while (osal_char_isspace(*ptr)) ptr++;
    if (next_ptr)
    {
        n = next_ptr - ptr;
        if (n > buf_sz) n = buf_sz;
        *list_str_ptr = next_ptr;
    }
    else
    {
        n = buf_sz;
        *list_str_ptr = OS_NULL;
    }

    os_strncpy(buf, ptr, n);
    q = buf + n;
    while (q-- > buf)
    {
        if (osal_char_isspace(*q)) *q = '\0';
        else break;
    }

    return OSAL_SUCCESS;
}

#endif
