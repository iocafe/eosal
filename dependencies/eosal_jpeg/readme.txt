eosal_jpeg - libjpeg 
notes 3.5.2020/pekka

This is the original old libjpeg, jpegsr6.zip, dated 6b of 27.3.1998 with minor changes to organization.

- Memory allocation uses eosal functions
- Global external names staring with "esoal_" are used to allow linking into program which already links with another version of libjpeg
- Folder organization, cmake, visual studio, and arduino copy script as in other eosal/iocom libraries.
- Only JPEG format support is compiled in.



 jpeg libThis 
gcc -O2  -I.   -c -o jcapimin.o jcapimin.c
gcc -O2  -I.   -c -o jcapistd.o jcapistd.c
gcc -O2  -I.   -c -o jctrans.o jctrans.c
gcc -O2  -I.   -c -o jcparam.o jcparam.c
gcc -O2  -I.   -c -o jdatadst.o jdatadst.c
gcc -O2  -I.   -c -o jcinit.o jcinit.c
gcc -O2  -I.   -c -o jcmaster.o jcmaster.c
gcc -O2  -I.   -c -o jcmarker.o jcmarker.c
gcc -O2  -I.   -c -o jcmainct.o jcmainct.c
gcc -O2  -I.   -c -o jcprepct.o jcprepct.c
gcc -O2  -I.   -c -o jccoefct.o jccoefct.c
gcc -O2  -I.   -c -o jccolor.o jccolor.c
gcc -O2  -I.   -c -o jcsample.o jcsample.c
gcc -O2  -I.   -c -o jchuff.o jchuff.c
gcc -O2  -I.   -c -o jcphuff.o jcphuff.c
gcc -O2  -I.   -c -o jcdctmgr.o jcdctmgr.c
gcc -O2  -I.   -c -o jfdctfst.o jfdctfst.c
gcc -O2  -I.   -c -o jfdctflt.o jfdctflt.c
gcc -O2  -I.   -c -o jfdctint.o jfdctint.c
gcc -O2  -I.   -c -o jdapimin.o jdapimin.c
gcc -O2  -I.   -c -o jdapistd.o jdapistd.c
gcc -O2  -I.   -c -o jdtrans.o jdtrans.c
gcc -O2  -I.   -c -o jdatasrc.o jdatasrc.c
gcc -O2  -I.   -c -o jdmaster.o jdmaster.c
gcc -O2  -I.   -c -o jdinput.o jdinput.c
gcc -O2  -I.   -c -o jdmarker.o jdmarker.c
gcc -O2  -I.   -c -o jdhuff.o jdhuff.c
gcc -O2  -I.   -c -o jdphuff.o jdphuff.c
gcc -O2  -I.   -c -o jdmainct.o jdmainct.c
gcc -O2  -I.   -c -o jdcoefct.o jdcoefct.c
gcc -O2  -I.   -c -o jdpostct.o jdpostct.c
gcc -O2  -I.   -c -o jddctmgr.o jddctmgr.c
gcc -O2  -I.   -c -o jidctfst.o jidctfst.c
gcc -O2  -I.   -c -o jidctflt.o jidctflt.c
gcc -O2  -I.   -c -o jidctint.o jidctint.c
gcc -O2  -I.   -c -o jidctred.o jidctred.c
gcc -O2  -I.   -c -o jdsample.o jdsample.c
gcc -O2  -I.   -c -o jdcolor.o jdcolor.c
gcc -O2  -I.   -c -o jquant1.o jquant1.c
gcc -O2  -I.   -c -o jquant2.o jquant2.c
gcc -O2  -I.   -c -o jdmerge.o jdmerge.c
gcc -O2  -I.   -c -o jcomapi.o jcomapi.c
gcc -O2  -I.   -c -o jutils.o jutils.c
gcc -O2  -I.   -c -o jerror.o jerror.c

