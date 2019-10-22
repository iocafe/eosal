# eosal/build/arduino-library/CmakeLists.txt - Cmake build for eosal library.
cmake_minimum_required(VERSION 3.5)

# Set some informative project name.
project("arduino-library")

# Set project name (name root) and operating system (here arduino for Arduino IDE).
set(E_PROJECT "eosal")
set(ENV{E_OS} "arduino")
if (NOT DEFINED ENV{E_ROOT})
  if(WIN32)
    set(ENV{E_ROOT} "c:/coderoot")
  else()
    set(ENV{E_ROOT} "/coderoot")
  endif()
endif()

# set resulting .zip file name
set (E_TARGET ${E_PROJECT}-master)

# Set path to source files.
set(BACKBONE "$ENV{E_ROOT}/${E_PROJECT}/code")
set(EXTENSIONS "$ENV{E_ROOT}/${E_PROJECT}/extensions")

# Add main headers.
set(MAIN_HEADERS "${E_PROJECT}.h" "${E_PROJECT}x.h")

# Add source files common to all operating systems.
file(GLOB BB_CM_SOURCES RELATIVE $ENV{E_ROOT}/${E_PROJECT} "${BACKBONE}/*/common/*.c")
file(GLOB BB_CM_HEADERS RELATIVE $ENV{E_ROOT}/${E_PROJECT} "${BACKBONE}/*/common/*.h")
file(GLOB EX_CM_SOURCES RELATIVE $ENV{E_ROOT}/${E_PROJECT} "${EXTENSIONS}/*/common/*.c")
file(GLOB EX_CM_HEADERS RELATIVE $ENV{E_ROOT}/${E_PROJECT} "${EXTENSIONS}/*/common/*.h")

# Add operating system specific files.
file(GLOB BB_OS_SOURCES RELATIVE $ENV{E_ROOT}/${E_PROJECT} "${BACKBONE}/*/$ENV{E_OS}/*.c" "${BACKBONE}/*/$ENV{E_OS}/*.cpp")
file(GLOB BB_OS_HEADERS RELATIVE $ENV{E_ROOT}/${E_PROJECT} "${BACKBONE}/*/$ENV{E_OS}/*.h")
file(GLOB EX_OS_SOURCES RELATIVE $ENV{E_ROOT}/${E_PROJECT} "${EXTENSIONS}/*/$ENV{E_OS}/*.c" "${EXTENSIONS}/*/$ENV{E_OS}/*.cpp")
file(GLOB EX_OS_HEADERS RELATIVE $ENV{E_ROOT}/${E_PROJECT} "${EXTENSIONS}/*/$ENV{E_OS}/*.h")

# Generate file lists with relative paths.
set (E_FILE_LIST ${BB_CM_SOURCES} ${BB_OS_SOURCES} ${EX_CM_SOURCES} ${EX_OS_SOURCES})
set (E_HDR_FILE_LIST ${MAIN_HEADERS} ${BB_CM_HEADERS} ${BB_OS_HEADERS} ${EX_CM_HEADERS} ${EX_OS_HEADERS})

# Generate file list with absolute paths.
set(E_ABS_FILE_LIST)
foreach(E_FILE_NAME ${E_FILE_LIST})
  get_filename_component(E_ABS_FILE_NAME ${E_FILE_NAME} ABSOLUTE BASE_DIR "$ENV{E_ROOT}/${E_PROJECT}/")
  list(APPEND E_ABS_FILE_LIST ${E_ABS_FILE_NAME})
endforeach(E_FILE_NAME)
foreach(E_FILE_NAME ${E_HDR_FILE_LIST})
  get_filename_component(E_ABS_FILE_NAME ${E_FILE_NAME} ABSOLUTE BASE_DIR "$ENV{E_ROOT}/${E_PROJECT}/")
  list(APPEND E_ABS_FILE_LIST ${E_ABS_FILE_NAME})
endforeach(E_FILE_NAME)

# Add Arduino library info files.
set(E_INFO_FILE_LIST)
set (E_FILE_NAME "build/arduino-library/library.json")
list(APPEND E_INFO_FILE_LIST ${E_FILE_NAME})
get_filename_component(E_ABS_FILE_NAME ${E_FILE_NAME} ABSOLUTE BASE_DIR "$ENV{E_ROOT}/${E_PROJECT}")
list(APPEND E_ABS_FILE_LIST ${E_ABS_FILE_NAME})
set (E_FILE_NAME "build/arduino-library/library.properties")
list(APPEND E_INFO_FILE_LIST ${E_FILE_NAME})
get_filename_component(E_ABS_FILE_NAME ${E_FILE_NAME} ABSOLUTE BASE_DIR "$ENV{E_ROOT}/${E_PROJECT}")
list(APPEND E_ABS_FILE_LIST ${E_ABS_FILE_NAME})

# Make sure that target directory exists.
file(MAKE_DIRECTORY $ENV{E_ROOT}/lib/$ENV{E_OS})
file(REMOVE $ENV{E_ROOT}/lib/$ENV{E_OS}/${E_TARGET}.zip)

# Generate zipped arduino library.
# powershell.exe -nologo -noprofile -command "& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('foo.zip', 'bar'); }"
# Add-Type -A System.IO.Compression.FileSystem
# [IO.Compression.ZipFile]::CreateFromDirectory('foo', 'foo.zip')
# [IO.Compression.ZipFile]::ExtractToDirectory('foo.zip', 'bar')
add_custom_target(${E_TARGET} ALL
    DEPENDS ${E_ABS_FILE_LIST}
    COMMAND zip -j $ENV{E_ROOT}/lib/$ENV{E_OS}/${E_TARGET} ${E_FILE_LIST}
    COMMAND zip $ENV{E_ROOT}/lib/$ENV{E_OS}/${E_TARGET} ${E_HDR_FILE_LIST}
    COMMAND zip -j $ENV{E_ROOT}/lib/$ENV{E_OS}/${E_TARGET} ${E_INFO_FILE_LIST}
    WORKING_DIRECTORY $ENV{E_ROOT}/${E_PROJECT}
    )

