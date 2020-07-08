# make_debian_package.py 8.7.2020/pekka
# Make debian installation package for linux.
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

def runcmd(cmd):
    stream = os.popen(cmd)
    output = stream.read()
    print(output)    

def write_control_file(DEBIAN_path, appname, description, arch, organization, version):
    tmpctrl = '/tmp/' + organization + '-' + appname + '-' + version + '-' + arch + '-control.tmp'
    finalctrl = DEBIAN_path + "/control"
    ctrlfile = open(tmpctrl, "w")
    ctrlfile.write('Package: ' + organization + appname + '\n')
    ctrlfile.write('Version: ' + version  + '\n')
    ctrlfile.write('Section: custom\n')
    ctrlfile.write('Priority: optional\n')
    ctrlfile.write('Architecture: ' + arch + '\n')
    ctrlfile.write('Essential: no\n')
    ctrlfile.write('Installed-Size: 1024\n')
    ctrlfile.write('Maintainer: ' + organization + '\n')
    ctrlfile.write('Description: ' + description + '\n')
    ctrlfile.close()
    runcmd('sudo mv ' + tmpctrl + ' ' + finalctrl)

def make_debian_package(sourcepath, appname, description, sysname, arch, organization):
    if platform.system() == 'Windows':
        if not sourcepath.startswith("c:"):
            sourcepath = 'c:' + sourcepath
        coderoot = 'c:/coderoot'
    else:
        coderoot = '/coderoot'
    packages_path = coderoot + '/packages' + '/' + sysname
    pack_name = sysname + '-' + arch
    # pack_root = sourcepath + '/pack'
    pack_root = '/tmp/iocafe-pack'
    pack_path = pack_root + '/' + pack_name
    DEBIAN_path = pack_path + '/DEBIAN'
    mymakedir(DEBIAN_path)

    version_path = coderoot + '/eosal'
    version = read_version_txt(version_path)

    write_control_file(DEBIAN_path, appname, description, arch, organization, version)

    target_bin = pack_path + '/coderoot/' + organization + '/bin/' + sysname
    mymakedir(target_bin)
    target_file = target_bin + '/' + organization + '-' + appname

    # Here /coderoot/bin/linux path doesn't include architecture. Perhaps it should, this
    # would be necessary for separating cross compilations to different architectures.
    src_bin = coderoot + '/bin/' + sysname 
    source_file = src_bin + '/' + appname

    runcmd('sudo cp ' + source_file + ' ' + target_file)
    print ('executable file ' + source_file + ' copied into package')

    runcmd('sudo chown --recursive root ' + pack_path + '/*')
    runcmd('sudo chgrp --recursive root ' + pack_path + '/*')
    runcmd('sudo chmod --recursive 0755 ' + pack_path + '/*')
    runcmd('sudo chmod --recursive 04755 ' + target_bin + '/*')

    os.chdir(pack_root)
    runcmd('dpkg-deb --build ' + pack_name)

    mymakedir(packages_path)
    runcmd('mv ' + pack_root + '/' + pack_name + '.deb ' +  packages_path + '/' + organization + '-' + appname + '-' + version + '-' + sysname + '-' + arch + '.deb')

    runcmd('sudo rm -Rf ' + pack_path)

if __name__ == "__main__":
    # Get command line arguments
    appname = 'app'
    description = 'iocafe application'
    sysname = 'linux'
    arch = 'amd64'
    organization = 'iocafe'
    sourcepath = None
    expect = None
    isfirst = True
    for a in sys.argv:
        if isfirst:
            isfirst = False

        elif a == "-?" or a == "-help" or a == "--help":
            print ('python3 /coderoot/eosal/scripts/copy_package.py /tmp/.../firmware.bin -a candy -s esp32 -h espcam -o iocafe')

        elif a[0] == "-":
            expect = a[1]

        else:
            if expect == None:
                sourcepath = a

            if expect == "a":
                appname = a
                expect = None

            if expect == "d":
                description = a
                expect = None

            if expect == "s":
                sysname = a
                expect = None

            if expect == "h":
                arch = a
                expect = None

            if expect == "o":
                organization = a
                expect = None

    if sourcepath == None:
        print("No source file")
        exit()

    make_debian_package(sourcepath, appname, description, sysname, arch, organization)
