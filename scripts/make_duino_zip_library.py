#!/usr/bin/env python3
# make_duino_zip_library.py 25.1.2021/pekka
# Create a zip library useful which can be installed by Arduino IDE.
from os import listdir, makedirs
from os.path import isfile, isdir, join, splitext, exists
from shutil import copyfile
import sys

def mymakedir(targetdir):
    if not exists(targetdir):
        makedirs(targetdir)

def copy_level_2(sourcedir,roottargetdir,targetdir):
    files = listdir(sourcedir)

    # Copy header files
    for f in files:
        p = join(sourcedir, f)
        if isfile(p):
            e = splitext(p)[1]
            if e == '.h':
                mymakedir(targetdir)
                t = join(targetdir, f)
                copyfile(p, t)
            if e == '.c' or e == '.cpp':
                t = join(roottargetdir, f)
                copyfile(p, t)

def copy_info(f,sourcedir,targetdir):
    infodir = sourcedir + '/build/arduino-library'
    p = join(infodir, f)
    t = join(targetdir, f)
    if exists(p):
        copyfile(p, t)

def copy_level_1(sourcedir,targetdir):
    mymakedir(targetdir)
    files = listdir(sourcedir)

    # Copy header files
    for f in files:
        p = join(sourcedir, f)
        if isfile(p):
            e = splitext(p)[1]
            if e == '.h':
                t = join(targetdir, f)
                copyfile(p, t)

    # Copy code and extensions folders
    copy_level_2(sourcedir + '/code', targetdir, targetdir + '/code')
    copy_level_2(sourcedir + '/extensions/dynamicio', targetdir, targetdir + '/extensions/dynamicio')

    # Copy informative arduino files
    copy_info('library.json', sourcedir, targetdir)
    copy_info('library.properties', sourcedir, targetdir)

import shutil
shutil.make_archive(output_filename, 'zip', dir_name)

def mymain():
    outdir = "/coderoot/lib/arduino-platformio/iocom"
    expectplatform = True
    n = len(sys.argv)
    for i in range(1, n):
        if sys.argv[i][0] == "-":
            if sys.argv[i][1] == "o":
                expectplatform = False

        else:
            if not expectplatform:
                outdir = sys.argv[i];

            expectplatform = True

    copy_level_1("/coderoot/iocom", outdir)

# Usage make_duino_zip_library.py -o /coderoot/lib/esp32/iocom
mymain()


