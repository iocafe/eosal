# eosal/CmakeLists.txt - Cmake build for the EOSAL library, 10.3.2026/pekka.
cmake_minimum_required(VERSION 3.12)

# Set project name (= project root folder name).
set(E_PROJECT "eosal")

# Select which TLS library, mbedtls or openssl to use for the OS.
# For now, MbedTLS is the only supported TLS library, but we may add OpenSSL support later.
set(E_TLSLIB "mbedtls")

# Set build root environment variable E_ROOT
include("${CMAKE_CURRENT_LIST_DIR}/osbuild/cmakedefs/eosal-root-path.txt")

if (DEFINED ESP_PLATFORM)
  # Set path to source files.
  set(BACKBONE "$ENV{E_ROOT}/${E_PROJECT}/code")
  set(EXTENSIONS "$ENV{E_ROOT}/${E_PROJECT}/extensions")

  # List source files common to all operating systems. We do not need header files for now.
  file(GLOB BB_CM_SOURCES "${BACKBONE}/*/common/*.c")
  file(GLOB EX_CM_SOURCES "${EXTENSIONS}/*/common/*.c")

  # List ESP32, mbedTLS and FreeRTOS specific source files. We do not need header files for now.
  set(E_MTLIB "freertos")
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
  
  # Register eosal as ESP-IDF component.
  idf_component_register(SRCS "${srcs}" 
    INCLUDE_DIRS "${includes}" 
    REQUIRES "${requires}" )

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
