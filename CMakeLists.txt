# eosal/CmakeLists.txt - Cmake build for the EOSAL library, 5.9.2021/pekka.
cmake_minimum_required(VERSION 3.5)

# Set project name (= project root folder name).
set(E_PROJECT "eosal")

# Select which TLS library, mbedtls or openssl to use for the OS.
set(E_TLSLIB "mbedtls")

# If we are building for esp 
# if (NOT DEFINED ESP_PLATFORM)
#  message("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX")
#else()
#  message("xzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz")
# endif()

# set(ENV{E_ESPIDF} 1)

# Set build root environment variable E_ROOT
include("osbuild/cmakedefs/eosal-root-path.txt")

if (DEFINED ESP_PLATFORM)
  # Cmake Source Directories
  set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/eosal_esp32)

  include($ENV{IDF_PATH}/tools/cmake/project.cmake)
  project(${E_PROJECT})

else()
  project(${E_PROJECT})

  # include build information common to all projects.
  include("osbuild/cmakedefs/eosal-defs.txt")

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
