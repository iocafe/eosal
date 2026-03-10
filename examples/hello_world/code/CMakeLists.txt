# eosal/examples/hello_world/code/CmakeLists.txt, 10.3.2026/pekka
#
# ESP-IDF build only: The second CMakeLists.txt in src_dir (code) is responsible for controlling 
# the build process of the component and its integration into the overall project. 
# The component CMakeLists.txt file lists source files and registers the component to the build 
# system using idf_component_register.
#
cmake_minimum_required(VERSION 3.5)

# Project name, development root folder E_ROOT and eosal name for operatig system.
set(E_PROJECT "hello_world")
include("${COMPONENT_PATH}/../../../osbuild/cmakedefs/eosal-root-path.txt")

include("$ENV{E_ROOT}/eosal/osbuild/cmakedefs/eosal-defs-espidf.txt")

# Set path to source files.
set(E_MODULEDIR "$ENV{E_ROOT}/examples/${E_PROJECT}")

# List source files common to all operating systems. We do not need header files for now.
file(GLOB E_MODULE_SOURCES "${E_MODULEDIR}/code")

# Merge the .c files as one list. 
set(srcs ${E_MODULE_SOURCES})
# list(APPEND srcs ${EX_CM_SOURCES})

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
