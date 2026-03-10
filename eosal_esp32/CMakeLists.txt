# eosal/eosal_esp32/CmakeLists.txt, 4.9.2021/pekka
#
# ESP-IDF build only: The second CMakeLists.txt in src_dir (code) is responsible for controlling 
# the build process of the component and its integration into the overall project. 
# The component CMakeLists.txt file lists source files and registers the component to the build 
# system using idf_component_register.
#
# In ESP_IDFthe directory name is component name: The ESP-IDF build system identifies components 
# based on the name of the directory containing the CMakeLists.txt file. Here, a directory named 
# eosal_esp32 is treated as the component eosal_esp32.
#
# Since eosal code is placed in "code" and "extensions" folders for other build system, new directory 
# eosal_esp32 holds the CmakeList.txt which compiles files in code and extensions directories.
#
cmake_minimum_required(VERSION 3.9)


# Project name, development root folder E_ROOT and eosal name for operatig system.
set(E_PROJECT "eosal")
include("${COMPONENT_PATH}/../osbuild/cmakedefs/eosal-root-path.txt")

# We always use mbedtls and FreeRTOS libraries with ESP-IDF, select related files to build.
set(E_TLSLIB "mbedtls")
set(E_MTLIB "freertos")

include("${COMPONENT_PATH}/../osbuild/cmakedefs/eosal-defs-espidf.txt")



# Set path to source files.
set(BACKBONE "$ENV{E_ROOT}/${E_PROJECT}/code")
set(EXTENSIONS "$ENV{E_ROOT}/${E_PROJECT}/extensions")

# Main headers. We do not need header files for now.
# set(MAIN_HEADERS "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}.h" "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}x.h")

# List source files common to all operating systems. We do not need header files for now.
file(GLOB BB_CM_SOURCES "${BACKBONE}/*/common/*.c")
file(GLOB EX_CM_SOURCES "${EXTENSIONS}/*/common/*.c")
# file(GLOB BB_CM_HEADERS "${BACKBONE}/*/common/*.h")
# file(GLOB EX_CM_HEADERS "${EXTENSIONS}/*/common/*.h")

# List ESP32, mbedTLS and FreeRTOS specific source files. We do not need header files for now.
file(GLOB BB_OS_SOURCES "${BACKBONE}/*/$ENV{E_OS}/*.c" "${BACKBONE}/*/$ENV{E_OS}/*.cpp" "${BACKBONE}/*/${E_MTLIB}/*.c" "${BACKBONE}/*/${E_MTLIB}/*.cpp")
file(GLOB EX_OS_SOURCES "${EXTENSIONS}/*/$ENV{E_OS}/*.c" "${EXTENSIONS}/*/$ENV{E_OS}/*.cpp" "${EXTENSIONS}/*/${E_TLSLIB}/*.c" "${EXTENSIONS}/*/${E_TLSLIB}/*.cpp")
# file(GLOB BB_OS_HEADERS "${BACKBONE}/*/$ENV{E_OS}/*.h")
# file(GLOB EX_OS_HEADERS "${EXTENSIONS}/*/$ENV{E_OS}/*.h")

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


set(includes $ENV{E_ROOT}/eosal)
list(APPEND includes "$ENV{IDF_PATH}/components/mbedtls/mbedtls/include")

# Register eosal as ESP-IDEF component.
# set(COMPONENT_NAME "${E_PROJECT}")
idf_component_register(SRCS "${srcs}" 
    INCLUDE_DIRS "${includes}" 
    REQUIRES "${requires}" )
