# eosal/examples/ecollection/CmakeLists.txt - example collection build.
cmake_minimum_required(VERSION 3.12)

# Set project name (= project root folder name).
set(E_PROJECT "ecollection")

# Set build root environment variable E_ROOT.
include("${CMAKE_CURRENT_LIST_DIR}/../../osbuild/cmakedefs/eosal-root-path.txt")

if (DEFINED ESP_PLATFORM)
  # Include ESP-IDF build specific eosal settings for the top level cmake. 
  include("$ENV{E_ROOT}/eosal/osbuild/cmakedefs/eosal-top-level-espidf-setup.txt")

  # Set extra component directories and add code directory of this componenet.
  set(EXTRA_COMPONENT_DIRS ${E_MINIMAL_COMPONENT_DIRS})
  list(APPEND EXTRA_COMPONENT_DIRS $ENV{E_ROOT}/eosal/examples/${E_PROJECT}/code)

  # Include ESP-IDFbuild stuff. This will call the project() command and start the build process.
  include($ENV{IDF_PATH}/tools/cmake/project.cmake)
 
  include_directories("${CMAKE_SOURCE_DIR}")

  # Name the project.
  project(${E_PROJECT})

else()
  project(${E_PROJECT})

  # include build information common to all projects.
  include("$ENV{E_ROOT}/eosal/osbuild/cmakedefs/eosal-defs.txt")

  # List of libraries for linker
  set(E_APPLIBS "$ENV{OSAL_TLS_APP_LIBS}")
  
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

  # Build executable. Set library folder and libraries to link with.
  link_directories($ENV{E_LIB})
  add_executable(${E_PROJECT}${E_POSTFIX} ${HEADERS} ${SOURCES})
  target_link_libraries(${E_PROJECT}${E_POSTFIX} ${E_APPLIBS})

endif()



