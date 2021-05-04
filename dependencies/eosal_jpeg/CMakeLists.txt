# iocom/CmakeLists.txt - Cmake build for iocom library, 26.4.2021/pekka.
cmake_minimum_required(VERSION 3.5)

# Set project name (= project root folder name).
set(E_PROJECT "eosal_jpeg")
project(${E_PROJECT})

# include build environment setting common to all projects.
include(../../osbuild/cmakedefs/eosal-defs.txt)

# Set path to where to keep libraries.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $ENV{E_LIBOUT})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $ENV{E_LIBOUT})

# Set path to source files.
set(BACKBONE "$ENV{E_ROOT}/eosal/dependencies/${E_PROJECT}/code")
set(EOSALCODE "$ENV{E_ROOT}/eosal/dependencies/${E_PROJECT}/eosal_code")

# Add libjpeg to include path.
include_directories("$ENV{E_ROOT}/eosal/dependencies/eosal_jpeg")

# Add main headers.
set(MAIN_HEADERS "$ENV{E_ROOT}/eosal/dependencies/${E_PROJECT}/${E_PROJECT}.h")

# Add source files common to all operating systems.
file(GLOB BB_SOURCES "${BACKBONE}/*.c")
file(GLOB BB_HEADERS "${BACKBONE}/*.h")
file(GLOB EC_SOURCES "${EOSALCODE}/*.c")
file(GLOB EC_HEADERS "${EOSALCODE}/*.h")

# Generate the static library.
add_library(${E_PROJECT}${E_POSTFIX} STATIC ${MAIN_HEADERS} ${BB_HEADERS} ${EC_HEADERS} ${BB_SOURCES} ${EC_SOURCES})
