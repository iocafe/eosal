notes 8.1.2020/pekka
Micro-controllers store persistent board configuration parameters in EEPROM or flash. On Windows/Linux 
these parameters are usually saved in files within OS file system.

There are various implementations: Flash and EEPROM hardware use is typically microcontroller specific, and common library functionality can rarely be used. On Arduino there is valuable effort to make this portable, but even with Arduino we cannot necessarily use the portable code: To be specific we cannot use Arduino EEPROM emulation relying on flash with secure flash program updates over TLS. 
On raw metal or custom SPI connected EEPROMS we always end up with micro-controller or board specific code.
On Windows or Linux this is usually simple, we can just save board configuration in file system. Except if we have read only file system on Linux. 

The "persistent" directory contains persistent storage interface header osal_persistent.h, which declares how the persistent storage access functions look like. The actual implementations are in subdirectories for different platforms, boards and setups.

The subdirectories named after platform, like "arduino", "linux" or "windows" contain either the persistent storage implementations for the platform or include C code from "shared" directory, when a generic implementation can be used. The "metal" subdirectory is used typically with micro-controller without any operating system.

Warning: Do not use micro-controller flash to save any data which changes during normal run time operation, it will eventually burn out the flash (death of the micro controller).




