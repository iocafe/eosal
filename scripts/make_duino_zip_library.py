#!/usr/bin/env python3
# make_duino_zip_library.py 25.1.2021/pekka
# Create a zip library useful which can be installed by Arduino IDE.
# from os import listdir, makedirs
from os import makedirs
# from os.path import isfile, isdir, join, splitext, exists
from os.path import exists
# from shutil import copyfile
import sys
from shutil import make_archive
from platform import system


def mymakedir(targetdir):
    if not exists(targetdir):
        makedirs(targetdir)

def read_version_txt(version_path):
    file_content = 'XXXXXX-XXXX'

    try:
        with open(version_path + "/eosal_version.txt", mode='r') as file: # b is important -> binary
            file_content = file.read()
    except:
        file_content = None
        pass            

    return file_content

def zip_ip(libname, indir, outdir):
    if system() == 'Windows':
        if not indir.startswith("c:"):
            indir = 'c:' + indir
        if not outdir.startswith("c:"):
            outdir = 'c:' + outdir
        coderoot = 'c:/coderoot'
    else:
        coderoot = '/coderoot'

    version_path = coderoot + '/eosal'
    version = read_version_txt(version_path)
    if version == None:
        outpath = outdir + "/" + libname
    else:
        outpath = outdir + "/" + version + "-" + libname

    make_archive(outpath, 'zip', indir)

def mymain():
    indir = "/coderoot/lib/arduino/eosal"
    outdir = "/tmp"
    libname = "unknown"
    expectoutdir  = False
    expectindir  = False
    n = len(sys.argv)
    for i in range(1, n):
        if sys.argv[i][0] == "-":
            if sys.argv[i][1] == "o":
                expectoutdir = True
            if sys.argv[i][1] == "i":
                expectindir = True

        else:
            if  expectoutdir:
                outdir = sys.argv[i]
            elif  expectindir:
                indir = sys.argv[i]
            else:
                libname = sys.argv[i]

            expectoutdir = False
            expectindir = False

    mymakedir(outdir)
    zip_ip(libname, indir, outdir)

# Usage make_duino_zip_library.py eosal -i /coderoot/lib/arduino/eosal -o /coderoot/lib/arduino-zips
mymain()


