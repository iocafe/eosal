/**

  @file    strcnv/common/osal_double_to_str.c
  @brief   Convert string to floating point number.
  @author  Pekka Lehtikoski
  @version 1.0
  @date    8.1.2020

  Function osal_str_to_double() converts string to floating point number.
  This code is adopted from code written by "Michael Ringgaard". Original copyright 
  note below.

  Copyright (C) 2002 Michael Ringgaard. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
   
  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.  
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.  
  3. Neither the name of the project nor the names of its contributors
     may be used to endorse or promote products derived from this software
     without specific prior written permission. 
   
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
  SUCH DAMAGE.

****************************************************************************************************
*/
#include "eosalx.h"
#if OSAL_STRCONV_SUPPORT


/**
****************************************************************************************************

  @brief Convert string to floating point number.
  @anchor osal_str_to_double

  The osal_str_to_double() function converts a string to double precision floating point
  value. Preceeding white space characters are skipped. If the string doesn't contain value, 
  the function return 0 and count is set to 0.

  @param   str Pointer to string to parse.
  @param   count Pointer to integer where to store number of bytes parsed. Zero if the
           function failed. Can be OS_NULL if not needed.

  @return  String as double precision floating point number.

****************************************************************************************************
*/
os_double osal_str_to_double(
    const os_char *str,
    os_memsz *count)
{
    os_double number, p10;
    os_int exponent, negative, n, num_digits, num_decimals;
    os_char *p;

    p = (os_char*)str;

    /* Skip leading white space.
     */
    while (osal_char_isspace(*p)) p++;

    /* Handle optional sign
     */
	negative = 0;
	switch (*p) 
	{             
		case '-': 
			negative = 1; 
			/* Fall through to increment position.
			 */

		case '+': 
			p++;

			/* Skip white space between sign and first digit.
			 */
			while (osal_char_isspace(*p)) p++;
	}

	number = 0.;
	exponent = 0;
	num_digits = 0;
	num_decimals = 0;

	/* Process string of digits.
	 */
	while (osal_char_isdigit(*p))
	{
		number = number * 10. + (*p - '0');
		p++;
		num_digits++;
	}

	/* Process decimal part.
	 */
	if (*p == '.') 
	{
		p++;

		while (osal_char_isdigit(*p))
		{
			number = number * 10. + (*p - '0');
			p++;
			num_digits++;
			num_decimals++;
		}

		exponent -= num_decimals;
	}

	if (num_digits == 0)
	{
		goto getout;
	}

	/* Correct for sign.
	 */
	if (negative) number = -number;

	/* Process an exponent string.
	 */
	if (*p == 'e' || *p == 'E') 
	{
		/* Handle optional sign.
		 */
		negative = 0;
		switch(*++p) 
		{   
			case '-': 
				negative = 1;   
				/* Fall through to increment pos.
				 */

			case '+': 
				p++;
		}

		/* Process string of digits.
		 */
	    n = 0;
		while (osal_char_isdigit(*p)) 
		{   
			n = n * 10 + (*p - '0');
			p++;
		}

		if (negative) 
			exponent -= n;
		else
			exponent += n;
	}

	/* if (exponent < DBL_MIN_EXP  || exponent > DBL_MAX_EXP)
	{
		goto getout;
	}
	*/

	/* Scale the result.
	 */
	p10 = 10.;
	n = exponent;
	if (n < 0) n = -n;
	while (n) 
	{
		if (n & 1) 
		{
			if (exponent < 0)
				number /= p10;
			else
				number *= p10;
		}
		n >>= 1;
		p10 *= p10;
	}

	/* if (number == HUGE_VAL) errno = ERANGE; */

    if (count) *count = (os_memsz)(p - str);
    return number;

getout:
    if (count) *count = 0;
    return 0.0;
}

#endif
