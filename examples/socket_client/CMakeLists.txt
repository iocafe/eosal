# eosal/examples/socket_client/build/cmake/CmakeLists.txt - Cmake build for eosal socket client example.
cmake_minimum_required(VERSION 3.5)

# Set project name (= project root folder name).
set(E_PROJECT "socket_client")
project(${E_PROJECT})

# include build information common to all projects.
include(../../build/cmakedefs/eosal-defs.txt)

# Build individual library projects.
add_subdirectory($ENV{E_ROOT}/eosal "${CMAKE_CURRENT_BINARY_DIR}/eosal")

# Set path to where to keep libraries.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{E_BIN})

# Set path to source files.
set(E_SOURCE_PATH "$ENV{E_ROOT}/eosal/examples/${E_PROJECT}/code")

# Add header files, the file(GLOB_RECURSE...) allows for wildcards and recurses subdirs.
file(GLOB_RECURSE HEADERS "${E_SOURCE_PATH}/*.h")

# Add source files.
file(GLOB_RECURSE SOURCES "${E_SOURCE_PATH}/*.c")
 
# Build executable. Set library folder and libraries to link with
link_directories($ENV{E_LIB})
add_executable(${E_PROJECT}${E_POSTFIX} ${SOURCES})
target_link_libraries(${E_PROJECT}${E_POSTFIX} $ENV{OSAL_TLS_APP_LIBS})
