eosal_esp32 directory - The ESP32 build system workaround
10.3.2026/Pekka 

In ESP_IDFthe directory name is component name: The ESP-IDF build system identifies components based on the name of 
the directory containing the CMakeLists.txt file. Here, a directory named eosal_esp32 is treated as the component eosal_esp32.

Since eosal code is placed in "code" and "extensions" folders for other build system, new directory eosal_esp32 
is created. It holds the CmakeList.txt which compiles files in code and extensions directories.


