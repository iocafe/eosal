/**

  @file    tls/common/osal_make_password.c
  @brief   Cryptographic hash.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    20.1.2020

  Creating random password and converting binary data to password string.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

****************************************************************************************************
*/
#include "eosalx.h"


/**
****************************************************************************************************

  @brief Convert 6 bit integer to ascii char
  @anchor osal_group_to_asc

  The osal_group_to_asc() function...

  @param   x Integer value 0-63 to convert to ASCII character. Highest two bits are ignored.
  @return  Ascii character corresponding to integer value x: '0'-'9', 'a'-'z', 'A'-'Z', '_' or '-'

****************************************************************************************************
*/
static os_char osal_group_to_asc(
    os_uchar x)
{
    const os_uchar n_alpha = 'z' - 'a' + 1; /* 26 characters */
    x &= 0x3F;

    if (x < 10) return '0' + x;
    x -= 10;
    if (x < n_alpha) return 'a' + x;
    x -= n_alpha;
    if (x < n_alpha) return 'A' + x;
    x -= n_alpha;
    return x ? '-' : '_';
}


/**
****************************************************************************************************

  @brief Convert binary data to password string
  @anchor osal_bin_to_password

  The osal_bin_to_password() function converts binary data to password string and stores the
  result as string into buffer. This is used to both convert encrypted passwords to string
  and to convert random numbers to random password.

  @param   str Buffer where to store the string result. The buffer must be at least 45 bytes, or
           46 if prefix_with_excl_mark is OS_TRUE.
  @param   str_sz Size of the string buffer in bytes.
  @param   data Binary souce data.
  @param   data_sz Binary data size in bytes.
  @param   prefix_with_excl_mark OS_TRUE to prefix resulting string with exclamation mark '!".
           The exclamation mark is used to mark encrypted passwords.
  @return  None.

****************************************************************************************************
*/
void osal_password_bin2str(
    os_char *str,
    os_memsz str_sz,
    const void *data,
    os_memsz data_sz,
    os_boolean prefix_with_excl_mark)
{
    os_uchar *s, md[3*OSAL_HASH_3_GROUPS];
    os_char *p, buf[OSAL_HASH_STR_SZ];
    os_short count;

    /* Setup source data so that it has 33 bytes. 32 data bited and one 0 byte
     */
    os_memclear(md, sizeof(md));
    if (data_sz > sizeof(md))
    {
        osal_debug_error("Too much data for password");
        data_sz = sizeof(md);
    }
    os_memcpy(md, data, data_sz);

    /* Set up target string buffer so that it has enough space (9*4 + 2 = 36 bytes)
     */
    os_memclear(buf, OSAL_HASH_STR_SZ);
    p = buf;
    s = md;

    /* Convert to string format.
     */
    if (prefix_with_excl_mark) *(p++) = '!';
    count = OSAL_HASH_3_GROUPS;
    while (count--)
    {
        *(p++) = osal_group_to_asc(s[0]);
        *(p++) = osal_group_to_asc(s[0] >> 6 | s[1] << 2);
        *(p++) = osal_group_to_asc(s[1] >> 4 | s[2] << 4);
        *(p++) = osal_group_to_asc(s[2] >> 2);
        s += 3;
    }
    *(p++) = '\0';

#if OSAL_DEBUG
    if (os_strlen(buf) > str_sz)
    {
        osal_debug_error("Too small password string buffer");
    }
#endif
    os_strncpy(str, buf, str_sz);
}


/**
****************************************************************************************************

  @brief Generate a random password
  @anchor osal_make_random_password

  The osal_make_random_password() function generates a random password string. The random
  password includes 256 bits of entropia.

  @param   password Buffer where to store the resulting password string. This must be at
           least 45 bytes long.
  @param   password_sz Size of the buffer in bytes.
  @return  None.

****************************************************************************************************
*/
void osal_make_random_password(
    os_char *password,
    os_memsz password_sz)
{
    #define OSAL_RAND_PASSWD_N 4 /* 4 * 64 bits = 256 bits */
    os_long binbuf[OSAL_RAND_PASSWD_N];
    os_short i;

    for (i = 0; i < OSAL_RAND_PASSWD_N; i++)
    {
        binbuf[i] = osal_rand(0, 0);
    }

    osal_password_bin2str(password, password_sz,
        binbuf, sizeof(binbuf), OS_FALSE);
}
