# eosal/code/CmakeLists.txt, 4.9.2021/pekka
#
# ESP-IDF build only: The second CMakeLists.txt in src_dir (code) sis responsible for controlling 
# the build process of the component and its integration into the overall project. 
# The component CMakeLists.txt file lists source files and registers the component to the build 
# system using idf_component_register.
#
cmake_minimum_required(VERSION 3.16.0)

# Project name, development root folder and eosal name for operatig system.
set(E_PROJECT "eosal")
set(ENV{E_ROOT} "/coderoot")
set(ENV{E_OS} "esp32")

# We always use mbedtls and FreeRTOS libraries with ESP-IDF, select related files to build.
set(E_TLSLIB "mbedtls")
set(E_MTLIB "freertos")

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

# Merge the .c files as one list.
set(srcs ${BB_CM_SOURCES})
list(APPEND srcs ${EX_CM_SOURCES})
list(APPEND srcs ${BB_OS_SOURCES})
list(APPEND srcs ${EX_OS_SOURCES})

# The eosal library depends on: 
set(requires, "idf::mbedtls")

# Register eosal as ESP-IDEF component.
idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "." REQUIRES "${requires}" )
