# eosal/examples/code/CmakeLists.txt, 10.3.2026/pekka
#
# ESP-IDF build only: The second CMakeLists.txt in src_dir (code) is responsible for controlling 
# the build process of the component and its integration into the overall project. 
# The component CMakeLists.txt file lists source files and registers the component to the build 
# system using idf_component_register.
#
cmake_minimum_required(VERSION 3.5)

# Project name, development root folder E_ROOT and eosal name for operatig system.
set(E_PROJECT "hello_world")
set(ENV{E_OS} "esp32")
include("${COMPONENT_PATH}/../../../osbuild/cmakedefs/eosal-root-path.txt")

# We always use mbedtls and FreeRTOS libraries with ESP-IDF, select related files to build.
# set(E_TLSLIB "mbedtls")
# set(E_MTLIB "freertos")

include("$ENV{E_ROOT}/eosal/osbuild/cmakedefs/eosal-defs-espidf.txt")

# Add defines to indicate target operating system, OS version, and architecture (at level needed for build).
# add_definitions("-DE_OS_$ENV{E_OS}"")
# add_definitions(-DE_OSVER_$ENV{E_OSVER})
# add_definitions(-DE_ARCH_$ENV{E_ARCH})

# Set path to source files.
set(E_MODULEDIR "$ENV{E_ROOT}/examples/${E_PROJECT}")
# set(EXTENSIONS "$ENV{E_ROOT}/${E_PROJECT}/extensions")

# Main headers. We do not need header files for now.
# set(MAIN_HEADERS "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}.h" "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}x.h")

# List source files common to all operating systems. We do not need header files for now.
file(GLOB E_MODULE_SOURCES "${E_MODULEDIR}/code")
# file(GLOB EX_CM_SOURCES "${EXTENSIONS}/*/common/*.c")
# file(GLOB BB_CM_HEADERS "${BACKBONE}/*/common/*.h")
# file(GLOB EX_CM_HEADERS "${EXTENSIONS}/*/common/*.h")

# List ESP32, mbedTLS and FreeRTOS specific source files. We do not need header files for now.
# file(GLOB BB_OS_SOURCES "${BACKBONE}/*/$ENV{E_OS}/*.c" "${BACKBONE}/*/$ENV{E_OS}/*.cpp" "${BACKBONE}/*/${E_MTLIB}/*.c" "${BACKBONE}/*/${E_MTLIB}/*.cpp")
# file(GLOB EX_OS_SOURCES "${EXTENSIONS}/*/$ENV{E_OS}/*.c" "${EXTENSIONS}/*/$ENV{E_OS}/*.cpp" "${EXTENSIONS}/*/${E_TLSLIB}/*.c" "${EXTENSIONS}/*/${E_TLSLIB}/*.cpp")
# file(GLOB BB_OS_HEADERS "${BACKBONE}/*/$ENV{E_OS}/*.h")
# file(GLOB EX_OS_HEADERS "${EXTENSIONS}/*/$ENV{E_OS}/*.h")

# Merge the .c files as one list. On Windows it may be necessary to get rid of backslashes 
# (especially if coderoot is set as environment variable)
set(srcs ${E_MODULE_SOURCES})
# list(APPEND srcs ${EX_CM_SOURCES})
# list(APPEND srcs ${BB_OS_SOURCES})
# list(APPEND srcs ${EX_OS_SOURCES})

# The eosal library depends on: 
set(requires "eosal_esp32")
list(APPEND requires "mbedtls")
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
idf_component_register(SRCS "hello_world_example.c" "${srcs}" INCLUDE_DIRS "${includes}" REQUIRES "${requires}" )
