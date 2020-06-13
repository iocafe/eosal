# merge_json.py 8.5.2020/pekka
# Include JSON files within JSON.
import json
import os
import sys

def merge(merged, di):
    for item in di.items():
        key = item[0]
        value = item[1]
        mitem = merged.get(key, None)
        if mitem == None:
            if value != None:
                merged[key] = value
        else:
            if isinstance(mitem, list):
                for item in value:
                    mergelist(mitem, item)

            elif isinstance(mitem, dict):
                merge(mitem, value)

def mergelist(mlist, item):
    name = item.get('name', None)
    if name == None:
        print('List item without name?')
        return

    for m in mlist:
        mname = m.get('name', None)
        if mname == name:
            merge(m, item)
            return

    mlist.append(item)

def process_source_file(merged, path):
    read_file = open(path, "r")
    if read_file:
        data = json.load(read_file)
        merge(merged, data)
    else:
        print ("Opening file " + path + " failed")

def mymain():
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
        exit()

#        sourcefiles.append('/coderoot/iocom/examples/candy/config/parameters/parameters.json')
#        sourcefiles.append('/coderoot/iocom/config/parameters/wifi_dhcp_device_network_parameters.json')

#        sourcefiles.append('/coderoot/iocom/examples/candy/config/signals/signals.json')
#        sourcefiles.append('/coderoot/iocom/config/signals/device_conf_signals.json')
#        sourcefiles.append('/coderoot/iocom/config/signals/camera_buf8k_signals.json')

    # If output path is not given as argument.
    if outpath is None:
        path, file_extension = os.path.splitext(sourcefiles[0])
        dir_path, file_name =  os.path.split(path)
        outpath = dir_path + '/intermediate/' + file_name + '-merged' + file_extension

    # Make sure that "intermediate" directory exists.
    dir_path, file_name =  os.path.split(outpath)
    try:
        os.makedirs(dir_path)
    except FileExistsError:
        pass

    print("Writing file " + outpath)

    merged = {}
    for path in sourcefiles:
        process_source_file(merged, path)

    with open(outpath, "w") as outfile:
        json.dump(merged, outfile, indent=4)

mymain()
