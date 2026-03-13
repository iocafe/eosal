# eosal/examples/persistent/code/CmakeLists.txt, 10.3.2026/pekka
#
# ESP-IDF build only: The second CMakeLists.txt in project source directory "code" 
# controls the the build process of the main component. 
# This component CMakeLists.txt file lists source files and registers the component
# to the ESP32 build system using idf_component_register.
#
cmake_minimum_required(VERSION 3.12)

# Project name.
set(E_PROJECT "persistent")

# Set root folder E_ROOT for the build OS (/coderoot or c:/coderoot).
include("${COMPONENT_PATH}/../../../osbuild/cmakedefs/eosal-root-path.txt")

# Include build variable settings common to ESP32 projects.
include("$ENV{E_ROOT}/eosal/osbuild/cmakedefs/eosal-defs-espidf.txt")

# List source files for the project. We do not need header files for now.
file(GLOB srcs "${COMPONENT_PATH}/*.c")

# This project depends on: 
set(requires "eosal_esp32")

# Register eosal as ESP-IDEF component.
idf_component_register(SRCS "${srcs}" REQUIRES "${requires}" )
