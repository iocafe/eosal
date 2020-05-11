# preprocess_json.py 8.5.2020/pekka
# Include JSON files within JSON.
import json
import os
import sys
import re

def process_source_file(path):
    global cfile, hfile, array_list, parameter_list
    global device_name, define_list
    read_file = open(path, "r")
    if read_file:
        data = json.load(read_file)
        if device_name == None:
            device_name = data.get("name", "unnameddevice")
        check_valid_name("Device", device_name, IOC_NAME_SZ, False)

        array_list = []
        parameter_list = []

        pblk = data.get("persistent", None)
        if pblk != None:
            process_parameter_block('persistent', pblk)

        pblk = data.get("volatile", None)
        if pblk != None:
            process_parameter_block('volatile', pblk)

        pblk = data.get("network", None)
        if pblk != None:
            process_parameter_block('network', pblk)

        cfile.write('\n')
        '''
        cfile.write('};\n')

        hfile.write('}\n' + struct_name + ';\n\n')

        for d in define_list:
            hfile.write(d)

        hfile.write('\nextern OS_FLASH_MEM_H ' + struct_name + ' ' + device_name + ';\n')

        list_name = device_name + "_pblk_list"
        cfile.write('\nstatic OS_FLASH_MEM iocpblkparameterHdr * OS_FLASH_MEM ' + list_name + '[] =\n{\n  ')
        isfirst = True
        for p in pblk_list:
            if not isfirst:
                cfile.write(',\n  ')
            isfirst = False
            cfile.write('&' + p + '.hdr')
        cfile.write('\n};\n\n')
        cfile.write('OS_FLASH_MEM iocDeviceHdr ' + device_name + '_hdr = {(iocpblkparameterHdr**)' + list_name + ', sizeof(' + list_name + ')/' + 'sizeof(iocpblkparameterHdr*)};\n')
        hfile.write('extern OS_FLASH_MEM_H iocDeviceHdr ' + device_name + '_' + 'hdr;\n\n')

        if len(array_list) > 0:
            hfile.write('\n/* Array length defines. */\n')
            for p in array_list:
                hfile.write(p + '\n')

        if len(parameter_list) > 0:
            hfile.write('\n/* Defines to check in code with #ifdef to know if parameter is configured in JSON. */\n')
            for p in parameter_list:
                hfile.write(p + '\n')
        '''

    else:
        print ("Opening file " + path + " failed")

def mymain():
    global cfilepath, hfilepath, device_name

    # Get command line arguments
    n = len(sys.argv)
    sourcefiles = []
    outpath = None
    expectpath = True
    device_name = None
    for i in range(1, n):
        if sys.argv[i][0] == "-":
            if sys.argv[i][1] == "o":
                outpath = sys.argv[i+1]
                expectpath = False

            if sys.argv[i][1] == "d":
                device_name = sys.argv[i+1]
                expectpath = False

        else:
            if expectpath:
                sourcefiles.append(sys.argv[i])
            expectpath = True

    if len(sourcefiles) < 1:
        print("No source files")
#        exit()

    sourcefiles.append('/coderoot/iocom/examples/candy/config/parameters/parameters.json')
    outpath = '/coderoot/iocom/examples/candy/config/include/espcam/parameters.c'

    if outpath is None:
        outpath = sourcefiles[0]

    filename, file_extension = os.path.splitext(outpath)
    cfilepath = filename + '.c'
    hfilepath = filename + '.h'

    print("Writing files " + cfilepath + " and " + hfilepath)

    start_c_files()

    for path in sourcefiles:
        process_source_file(path)

    '''
    struct_name = device_name + '_t'
        define_list = []
        hfile.write('typedef struct ' + struct_name + '\n{')
        cfile.write('OS_FLASH_MEM struct ' + struct_name + ' ' + device_name + ' = \n{')
    '''

    hfile.write('\n#ifndef IOBOARD_DEVICE_NAME\n')
    hfile.write('#define IOBOARD_DEVICE_NAME \"' + device_name + '\"\n#endif\n')

    finish_c_files()

mymain()
