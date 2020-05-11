# bin_to_c.py 8.1.2020/pekka
# Converts a binary file to C character array.
import json
import os
import sys

def start_c_files():
    global cfile, hfile, cfilepath, hfilepath
    cfile = open(cfilepath, "w")
    hfile = open(hfilepath, "w")
    cfile.write('/* This file is gerated by bin_to_c.py script, do not modify. */\n')
    hfile.write('/* This file is gerated by bin_to_c.py script, do not modify. */\n')
    hfile.write('OSAL_C_HEADER_BEGINS\n\n')

def finish_c_files():
    global cfile, hfile
    hfile.write('\nOSAL_C_HEADER_ENDS\n')
    cfile.close()
    hfile.close()

def process_source_file(path, variablename):
    global cfile, hfile

    with open(path, mode='rb') as file: # b is important -> binary
        file_content = file.read()

        l = len(file_content)

        cfile.write('OS_FLASH_MEM os_char ' + variablename + '[' + str(l) + '] = {\n')
        hfile.write('extern OS_FLASH_MEM_H os_char ' + variablename + '[' + str(l) + '];\n')

        columns = 0
        for x in range(0, l):
            if columns >= 25:
                cfile.write("\n");
                columns = 0;
            cfile.write(str(file_content[x]));
            if x < l - 1:
                cfile.write(",")

            columns = columns + 1

        cfile.write('};\n')

def mymain():
    global cfilepath, hfilepath

    # Get command line arguments
    n = len(sys.argv)
    sourcefiles = []
    outpath = None
    variablename = "noame"
    expectpath = True
    for i in range(1, n):
        if sys.argv[i][0] == "-":
            if sys.argv[i][1] == "o":
                outpath = sys.argv[i+1]
                expectpath = False

            if sys.argv[i][1] == "v":
                variablename = sys.argv[i+1]
                expectpath = False

        else:
            if expectpath:
                sourcefiles.append(sys.argv[i])
            expectpath = True

    if len(sourcefiles) < 1:
        print("No source files")
        exit()

    if outpath is None:
        outpath = sourcefiles[0]

    filename, file_extension = os.path.splitext(outpath)
    cfilepath = filename + '.c'
    hfilepath = filename + '.h'

    print("Writing files " + cfilepath + " and " + hfilepath)

    start_c_files()

    for path in sourcefiles:
        process_source_file(path, variablename)

    finish_c_files()

mymain()
