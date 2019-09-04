/**

@file    examples/boos/ssl/ssl-test-main.cpp
@brief   Testing boost SSL.
@author  Pekka Lehtikoski
@version 1.0
@date    11.6.2016

Copyright 2012 - 2019 Pekka Lehtikoski. This file is part of the eosal and shall only be used,
modified, and distributed under the terms of the project licensing. By continuing to use, modify,
or distribute this file you indicate that you have read the license and understand and accept
it fully.

****************************************************************************************************
*/
#include <cstdlib>
#include <iostream>
#include "code/defs/osal_code.h"
#include "eosal/examples/boost/ssl/client.h"
#include "eosal/examples/boost/ssl/server.h"


/**
****************************************************************************************************

  @brief Process entry point.

  The osal_main() function is OS independent entry point.

  @param   argc Number of command line arguments.
  @param   argv Array of string pointers, one for each command line argument. UTF8 encoded.

  @return  None.

****************************************************************************************************
*/
os_int osal_main(
	os_int argc,
	os_char *argv[])
{
	if (argc < 2)
	{
		std::cerr << "Usage: ssl-test-main client/server <host> <port>\n";
		return 1;
	}

	if (argv[1][0] == 'c')
	{
		return client_main(argc - 1, argv + 1);
	}
	else {
		return server_main(argc - 1, argv + 1);
	}
}
