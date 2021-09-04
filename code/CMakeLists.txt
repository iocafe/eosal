# ESP-IDF build only: The second CMakeLists.txt in src_dir is responsible for controlling 
# the build process of the component and its integration into the overall project. 
# The minimal component CMakeLists.txt file simply registers the component to the build 
# system using idf_component_register 
#
# idf_component_register(SRCS "main.c" INCLUDE_DIRS "" REQUIRES idf::mbedtls)

# idf_component_register(SRCS "osal_espidf_dummy_app_main.c" "*/esp32/*.cpp" INCLUDE_DIRS "." REQUIRES idf::mbedtls REQUIRES "idf::log" REQUIRES "idf::freertos" REQUIRES "idf::esp_system")

# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16.0)

# include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

set(E_PROJECT "eosal")

# include build information common to all projects.
set(ENV{E_ROOT} "/coderoot")
set(ENV{E_OS} "esp32")
include("$ENV{E_ROOT}/eosal/osbuild/cmakedefs/eosal-defs.txt")

# Set path to where to keep libraries.
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $ENV{E_LIBOUT})
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $ENV{E_LIBOUT})

# Select which TLS library, mbedtls or openssl to use for the OS.
set(E_TLSLIB "mbedtls")

# Set path to source files.
set(BACKBONE "$ENV{E_ROOT}/${E_PROJECT}/code")
set(EXTENSIONS "$ENV{E_ROOT}/${E_PROJECT}/extensions")

# Add main headers.
set(MAIN_HEADERS "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}.h" "$ENV{E_ROOT}/${E_PROJECT}/${E_PROJECT}x.h")

# Add source files common to all operating systems.
file(GLOB BB_CM_SOURCES "${BACKBONE}/*/common/*.c")
file(GLOB BB_CM_HEADERS "${BACKBONE}/*/common/*.h")
file(GLOB EX_CM_SOURCES "${EXTENSIONS}/*/common/*.c")
file(GLOB EX_CM_HEADERS "${EXTENSIONS}/*/common/*.h")

# Add operating system specific files.
file(GLOB BB_OS_SOURCES "${BACKBONE}/*/$ENV{E_OS}/*.c" "${BACKBONE}/*/$ENV{E_OS}/*.cpp")
file(GLOB BB_OS_HEADERS "${BACKBONE}/*/$ENV{E_OS}/*.h")
file(GLOB EX_OS_SOURCES "${EXTENSIONS}/*/$ENV{E_OS}/*.c" "${EXTENSIONS}/*/$ENV{E_OS}/*.cpp" "${EXTENSIONS}/*/${E_TLSLIB}/*.c")
file(GLOB EX_OS_HEADERS "${EXTENSIONS}/*/$ENV{E_OS}/*.h")


set(srcs "osal_espidf_dummy_app_main.c")

list(APPEND srcs ${BB_CM_SOURCES})
list(APPEND srcs ${EX_CM_SOURCES})
list(APPEND srcs ${BB_OS_SOURCES})
list(APPEND srcs ${EX_OS_SOURCES})
# message(FATAL_ERROR "HERE ********** ${srcs}")
 
idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "." REQUIRES "idf::mbedtls" REQUIRES "idf::log" REQUIRES "idf::freertos")
