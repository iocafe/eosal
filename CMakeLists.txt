# eosal/CmakeLists.txt - Cmake build for the EOSAL library, 5.9.2021/pekka.
cmake_minimum_required(VERSION 3.5)

# Set project name (= project root folder name).
set(E_PROJECT "eosal")

# Select which TLS library, mbedtls or openssl to use for the OS.
set(E_TLSLIB "mbedtls")

if("${IDF_TARGET}" STREQUAL "esp32")
  # ESP-IFD only: Include IDF project setup and declare the project.
  include($ENV{IDF_PATH}/tools/cmake/project.cmake)
  project(${E_PROJECT})

  # include build information common to all projects.
  include(osbuild/cmakedefs/eosal-defs-espidf.txt)

else()
  project(${E_PROJECT})

  # include build information common to all projects.
  include(osbuild/cmakedefs/eosal-defs.txt)

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

  # Merge to single list. On Windows it may be necessary to get rid of backslashes 
  # (especially if coderoot is set as environment variable)
  set(srcs ${MAIN_HEADERS}) 
  list(APPEND srcs ${BB_CM_HEADERS})
  list(APPEND srcs ${BB_OS_HEADERS}) 
  list(APPEND srcs ${EX_CM_HEADERS})
  list(APPEND srcs ${EX_OS_HEADERS})
  list(APPEND srcs ${BB_CM_SOURCES})
  list(APPEND srcs ${BB_OS_SOURCES})
  list(APPEND srcs ${EX_CM_SOURCES})
  list(APPEND srcs ${EX_OS_SOURCES})
  cmake_path(CONVERT "${srcs}" TO_CMAKE_PATH_LIST srcs_nobackslashes)

  # Generate the static library
  add_library(${E_PROJECT}${E_POSTFIX} STATIC ${srcs_nobackslashes})

endif()
