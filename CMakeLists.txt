# eosal/CmakeLists.txt - Cmake build for the EOSAL library, 10.3.2026/pekka.
cmake_minimum_required(VERSION 3.12)

# Set project name (= project root folder name).
set(E_PROJECT "eosal")

# Select which TLS library, mbedtls or openssl to use for the OS.
# For now, MbedTLS is the only supported TLS library, but we may add OpenSSL support later.
set(E_TLSLIB "mbedtls")

# Set build root environment variable E_ROOT
include("${CMAKE_CURRENT_SOURCE_DIR}/osbuild/cmakedefs/eosal-root-path.txt")

if (DEFINED ESP_PLATFORM)
  # Include ESP-IDF build specific eosal settings for the top level cmake. 
  include("$ENV{E_ROOT}/eosal/osbuild/cmakedefs/eosal-top-level-espidf-setup.txt")

  # Enable dummy app_main for ESP-IDF build, so we can build eosal as executable ESP32 component. 
  add_definitions(-DOSAL_DUMMY_ESP32_APP_MAIN)

  # Include ESP-IDFbuild stuff. This will call the project() command and start the build process.
  include($ENV{IDF_PATH}/tools/cmake/project.cmake)
 
  # Name the project.
  project(${E_PROJECT})

else()
  project(${E_PROJECT})

  # Include common setting for most of builds.
  include("$ENV{E_ROOT}/eosal/osbuild/cmakedefs/eosal-defs.txt")
 
  # Set path to where to keep libraries.
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $ENV{E_LIBOUT})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $ENV{E_LIBOUT})

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

  # Merge to single list. 
  set(srcs ${MAIN_HEADERS}) 
  list(APPEND srcs ${BB_CM_HEADERS})
  list(APPEND srcs ${BB_OS_HEADERS}) 
  list(APPEND srcs ${EX_CM_HEADERS})
  list(APPEND srcs ${EX_OS_HEADERS})
  list(APPEND srcs ${BB_CM_SOURCES})
  list(APPEND srcs ${BB_OS_SOURCES})
  list(APPEND srcs ${EX_CM_SOURCES})
  list(APPEND srcs ${EX_OS_SOURCES})

  # Generate the static library
  add_library(${E_PROJECT}${E_POSTFIX} STATIC ${srcs})

endif()
