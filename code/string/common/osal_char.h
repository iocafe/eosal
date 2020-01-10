/**

  @file    string/common/osal_char.h
  @brief   Character classification and conversion.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  This header file contains macros and function prototypes for character classification
  and conversion.

  Copyright 2020 Pekka Lehtikoski. This file is part of the eosal and shall only be used, 
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept 
  it fully.

****************************************************************************************************
*/

/** 
****************************************************************************************************

  @name Character Classification Macros

  Each of these macros tests a specified character for satisfaction of a condition. 
  (By definition, the ASCII character set is a subset of all multibyte-character sets. 
  Macros osal_char_isaplha(), osal_char_islower() and osal_char_isupper() work only with
  English alphabet. 

****************************************************************************************************
 */
/*@{*/

/** The osal_char_isaplha() macro checks if c is an alphabetic character. The macro returns a 
    non-zero value if c is within the ranges A - Z or a - z.
 */
#define osal_char_isaplha(c) (os_boolean)(((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))

/** The osal_char_isdigit() macro checks if c is decimal digit. The macro returns a non-zero value
    if c is a decimal digit (0 - 9). 
 */
#define osal_char_isdigit(c) (os_boolean)((c) >= '0' && (c) <= '9')

/** The osal_char_islower() macro checks if c is lower case alphabetic ASCII character. The macro 
    returns a non-zero value if c is within range ('a' - 'z'). 
 */
#define osal_char_islower(c) (os_boolean)((c) >= 'a' && (c) <= 'z')

/** The osal_char_isupper() macro checks if c is upper case alphabetic ASCII character. The macro 
    returns a non-zero value if c is within range ('A' - 'Z'). 
 */
#define osal_char_isupper(c) (os_boolean)((c) >= 'A' && (c) <= 'Z')

/** The osal_char_isspace() macro checks if c is white space character. The macro returns a 
    non-zero value if c is within range (0x09 - 0x0D) or 0x20. 
 */
#define osal_char_isspace(c) (os_boolean)(((c) >= 0x09 && (c) <= 0x0D) || ((c) == 0x20))

/** The osal_char_isprint() macro checks if c is printable character. The macro returns a 
    non-zero value if c is within range (0x20 - 0x7E) or >= 0x80. 
 */
#define osal_char_isprint(c) (os_boolean)(((c) >= 0x20 && (c) <= 0x7E) || ((c) >= 0x80))

/*@}*/


/** 
****************************************************************************************************

  @name Character Conversion Macros

  Currently character conversion macros include only conversion of character to lower case
  osal_char_tolower() and conversion to upper case osal_char_toupper(). Both of these macros
  work only on English alphabet, not the whole unicode set.

****************************************************************************************************
 */
/*@{*/

/** The osal_char_tolower() macro makes sure that alphabetic ASCII character is lower case. 
    if c is within range ('A' - 'Z') the function returns corresponding lower case character. 
	Otherwise the function returns c as is.
 */
#define osal_char_tolower(c) (((c) >= 'A' && (c) <= 'Z') ? ((c) + ('a'-'A')) : (c))

/** The osal_char_toupper() macro makes sure that alphabetic ASCII character is upper case. 
    if c is within range ('a' - 'z') the function returns corresponding upper case character. 
	Otherwise the function returns c as is.
 */
#define osal_char_toupper(c) (((c) >= 'a' && (c) <= 'z') ? ((c) - ('a'-'A')) : (c))

/*@}*/
