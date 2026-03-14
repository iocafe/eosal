# eosal/eosal_esp32/CmakeLists.txt, 13.3.2026/pekka
#
# ESP-IDF build only: The second CMakeLists.txt in "eosal_esp32" is responsible for controlling 
# the build process of the component and its integration into the overall project. 
# The component CMakeLists.txt file lists source files and registers the component to the build 
# system using idf_component_register.
#
# In ESP_IDF the directory name is the component name: The ESP-IDF build system identifies components 
# based on the name of the directory containing the CMakeLists.txt file. Here, a directory named 
# eosal_esp32 is treated as the component eosal_esp32.
#
# Since eosal code is placed in "code" and "extensions" folders for other build system, new directory 
# eosal_esp32 holds the CmakeList.txt which compiles files in code and extensions directories.
#
cmake_minimum_required(VERSION 3.12)

# Project name, is development root folder name for the project.
set(E_PROJECT "eosal")

# Set E_ROOT environment variable to point to the project root folder. 
# This is used in the CMakeLists.txt files to locate source files and include paths.
include("${COMPONENT_PATH}/../osbuild/cmakedefs/eosal-root-path.txt")

# Include ESP32 specific setting for build.
include("${COMPONENT_PATH}/../osbuild/cmakedefs/eosal-defs-espidf.txt")

# Set path to source files.
set(BACKBONE "$ENV{E_ROOT}/${E_PROJECT}/code")
set(EXTENSIONS "$ENV{E_ROOT}/${E_PROJECT}/extensions")

# List source files common to all operating systems. We do not need header files for now.
file(GLOB BB_CM_SOURCES "${BACKBONE}/*/common/*.c")
file(GLOB EX_CM_SOURCES "${EXTENSIONS}/*/common/*.c")

# List ESP32, mbedTLS and FreeRTOS specific source files. We do not need header files for now.
set(E_MTLIB "freertos")
set(E_TLSLIB "mbedtls")
file(GLOB BB_OS_SOURCES "${BACKBONE}/*/$ENV{E_OS}/*.c" "${BACKBONE}/*/$ENV{E_OS}/*.cpp" "${BACKBONE}/*/${E_MTLIB}/*.c" "${BACKBONE}/*/${E_MTLIB}/*.cpp")
file(GLOB EX_OS_SOURCES "${EXTENSIONS}/*/$ENV{E_OS}/*.c" "${EXTENSIONS}/*/$ENV{E_OS}/*.cpp" "${EXTENSIONS}/*/${E_TLSLIB}/*.c" "${EXTENSIONS}/*/${E_TLSLIB}/*.cpp")

# Merge the .c files as one list. On Windows it may be necessary to get rid of backslashes 
# (especially if coderoot is set as environment variable)
set(srcs ${BB_CM_SOURCES})
list(APPEND srcs ${EX_CM_SOURCES})
list(APPEND srcs ${BB_OS_SOURCES})
list(APPEND srcs ${EX_OS_SOURCES})

# The eosal library depends on: 
set(requires, "mbedtls")
list(APPEND requires "freertos")
list(APPEND requires "spi_flash")
list(APPEND requires "bootloader_support")
list(APPEND requires "esp_timer")
list(APPEND requires "nvs_flash")
list(APPEND requires "esp_pm")
list(APPEND requires "esp_event")
list(APPEND requires "esp_wifi")
list(APPEND requires "app_update")
list(APPEND requires "esp_driver_gpio")
list(APPEND requires "esp_driver_uart")

list(APPEND requires "sock_utils")

# Set include paths
set(includes $ENV{E_ROOT}/eosal)
list(APPEND includes "$ENV{IDF_PATH}/components/mbedtls/mbedtls/include")
list(APPEND includes "$ENV{E_ROOT}/${E_PROJECT}/build/config")

# Register eosal as ESP-IDF component.
idf_component_register(SRCS "${srcs}" 
    INCLUDE_DIRS "${includes}" 
    REQUIRES "${requires}" )
