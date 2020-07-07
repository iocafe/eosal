# copy_package.py 7.7.2020/pekka
# Copy installation package from build folder to packages folder and rename it.
import sys
import os
import shutil
import platform

def read_version_txt(version_path):
    file_content = 'XXXXXX-XXXX'

    try:
        with open(version_path + "/eosal_version.txt", mode='r') as file: # b is important -> binary
            file_content = file.read()
    except:
        pass            

    return file_content

def mymakedir(path):
    try:
        os.makedirs(path)
    except:
        pass

def copy_package(sourcefile, appname, sysname, hw, organization):
    if platform.system() == 'Windows':
        version_path = 'c:/coderoot/eosal'
        target_path = 'c:/coderoot/packages'
    else:
        version_path = '/coderoot/eosal'
        target_path = '/coderoot/packages'

    version = read_version_txt(version_path)

    fname, ext = os.path.splitext(sourcefile)
    dstname = organization + '-' + appname + '-' + version + '-' + sysname + '-' + hw + ext
    dstdir = target_path + '/' + sysname
    mymakedir(dstdir)

    shutil.copy2(sourcefile, dstdir + '/' + dstname) 
    print ('package copied as ' + dstdir + '/' + dstname)

if __name__ == "__main__":
    # Get command line arguments
    appname = 'app'
    sysname = 'any'
    hw = 'generic'
    organization = 'iocafe'
    sourcefile = None
#    sourcefile = '/tmp/candy_ioboard_tmp/esp32doit-devkit-v1/firmware.bin'
    expect = None
    for a in sys.argv:
        if a == "-?" or a == "-help" or a == "--help":
            print ('python3 /coderoot/eosal/scripts/copy_package.py /tmp/.../firmware.bin -a candy -s esp32 -h espcam -o iocafe')

        elif a[0] == "-":
            expect = a[1]

        else:
            if expect == None:
                sourcefile = a

            if expect == "a":
                appname = a
                expect = None

            if expect == "s":
                sysname = a
                expect = None

            if expect == "h":
                hw = a
                expect = None

            if expect == "o":
                organization = a
                expect = None

    if sourcefile == None:
        print("No source file")
        exit()

    # execute only if run as a script
    copy_package(sourcefile, appname, sysname, hw, organization)
