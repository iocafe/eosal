eosal_jpeg - libjpeg 
notes 3.5.2021/pekka

This is the original old libjpeg, jpegsr6.zip, dated 6b of 27.3.1998 with minor changes to organization.

- Memory allocation uses eosal functions
- Global external names staring with "esoal_" are used to allow linking into program which already links with another version of libjpeg
- Folder organization, cmake, visual studio, and arduino copy script as in other eosal/iocom libraries.
- Only JPEG format support is compiled in.

