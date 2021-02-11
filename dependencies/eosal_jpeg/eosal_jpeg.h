/**

  @file    eosal_jpeg.h
  @brief   libjpeg
  @author  Pekka Lehtikoski
  @version 1.0
  @date    2.5.2020

  This is the original old libjpeg, jpegsr6.zip, dated 6b with minor changes
  to organization.

  - Memory allocation uses eosal (operating system abstraction layer) functions.
  - Global external names staring with "esoal_" are used to allow linking into program which
    already links with another version of libjpeg
  - Folder organization, cmake, visual studio, and arduino copy script as in other
    eosal/iocom libraries.
  - Only JPEG format support is compiled in.

  Copyright 2020 Pekka Lehtikoski. This file is part of the iocom project and shall only be used,
  modified, and distributed under the terms of the project licensing. By continuing to use, modify,
  or distribute this file you indicate that you have read the license and understand and accept
  it fully.

  Copyright (C) 1991-1998, Thomas G. Lane. This file is derived from work of the Independent
  JPEG Group's software.

****************************************************************************************************
*/
#ifndef EOSAL_JPEG
#define EOSAL_JPEG

/* Include operating system abstraction layer with extension headers.
 */
#include "eosalx.h"

/* If C++ compilation, all functions, etc. from this point on in included headers are
   plain C and must be left undecorated.
 */
OSAL_C_HEADER_BEGINS

#include "code/jinclude.h"
#include "code/jpeglib.h"
#include "code/jerror.h"		/* get library error codes too */
#include "eosal_code/osal_jpeg.h"

/* If C++ compilation, end the undecorated code.
 */
OSAL_C_HEADER_ENDS

#endif
