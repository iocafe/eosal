# eosal/examples/build/cmake/CmakeLists.txt - Cmake build for all eosal examples.
cmake_minimum_required(VERSION 2.8.11)
project(eosal-examples)

# include build information common to all projects.
include(../../../build/cmake/eosal-defs.txt)
 
# Build individual projects.
add_subdirectory($ENV{E_ROOT}/eosal/build/cmake "${CMAKE_CURRENT_BINARY_DIR}/eosal")
add_subdirectory($ENV{E_ROOT}/eosal/examples/socket_client/build/cmake "${CMAKE_CURRENT_BINARY_DIR}/socket_client_example")
add_subdirectory($ENV{E_ROOT}/eosal/examples/socket_server/build/cmake "${CMAKE_CURRENT_BINARY_DIR}/socket_server_example")
