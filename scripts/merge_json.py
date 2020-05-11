# preprocess_json.py 8.5.2020/pekka
# Include JSON files within JSON.
import json
import os
import sys
# import re

def merge(merged, dict):
    for item in dict.items():
        key = item[0]
        value = item[1]
        mitem = merged.get(key, None)
        if mitem == None:
            if value != None:
                merged[key] = value

def process_source_file(merged, path):
    read_file = open(path, "r")
    if read_file:
        data = json.load(read_file)
        merge(merged, data)
    else:
        print ("Opening file " + path + " failed")

def mymain():
    # Get command line arguments
    n = len(sys.argv)
    sourcefiles = []
    outpath = None
    expect = None
    for i in range(1, n):
        if sys.argv[i][0] == "-":
            if sys.argv[i][1] == "o":
                expect = 'o'
        else:
            if expect=='o':
                outpath = sys.argv[i]
                expect = None
            else:
                sourcefiles.append(sys.argv[i])

    if len(sourcefiles) < 1:
        print("No source files")
#        exit()

    sourcefiles.append('/coderoot/iocom/examples/candy/config/parameters/parameters.json')
    sourcefiles.append('/coderoot/iocom/config/parameters/wifi-dhcp-device-network-paameters.json')

    if outpath is None:
        filename, file_extension = os.path.splitext(sourcefiles[0])
        outpath = filename + '-merged' + file_extension

    print("Writing file " + outpath)

    merged = {}
    for path in sourcefiles:
        process_source_file(merged, path)

    with open(outpath, "w") as outfile: 
        json.dump(merged, outfile) 


mymain()
