notes 27.1.2020/pekka
The dependencies directory contains mostly pre compiled library dependencies (for convinience). 

Linux, OpenSSL: You can use libraries which come with your distribution, or ones here. Reason to use libraries from here is to link with a tested library version or link with library containing debug symbols.

Used liibrary paths, etc. set in /coderoot/eosal/build/cmakedefs/eosal-defs.txt.

The OpenSSL library build configuration (before make) 
./config --debug no-deprecated no-shared -static no-dgram no-legacy no-dtls no-tls1 no-tls1_1
./config --release no-deprecated no-shared -static no-dgram no-legacy no-dtls no-tls1 no-tls1_1


Windows, OpenSSL: In Windows we need to download and build manually every library dependency. For example OpenSSL, we need crypto and ssl libraries. I build copied the static libraries for built with Visual Studio 2019
and openssl headers here (in eosal repository). This enables compliling and linking with eosal defaults on other computer without bothering with openssl depenency.

WIZnetIOlibrary: It doesn't really belong here, but is for now, until better place is figured out.
