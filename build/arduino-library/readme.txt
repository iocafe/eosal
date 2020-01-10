Pack eosal as Arduino library.
notes 8.1.2020/pekka

The cmake file CMakeLists.txt in this directory packs source code as Arduino libary .zip file. 
The resulting library is intended to be used with teensyduino or stmduino (32 bit ARM microcontrollers). 

Usage:
  mkdir tmp
  cd tmp
  cmake ..
  make

The resulting .zip file will be in lib/arduino directory.
