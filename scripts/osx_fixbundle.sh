#!/bin/bash 
# usage osx_fixbundle.sh <bundle-name> <bzr_root>

if [[ ! -f version.h ]]; then 
    echo "**"
    echo "** ERROR: $0 doesn't seems to be launched from the kicad's bzr root !!!"
    echo "** Go in the bzr root directory and launch: scripts/osx_fixbundle.sh"
    echo "**"
    exit 1
fi

EXECUTABLES="`find . -name '*.app'`"



#
# Copies libraries under <bzr_root> in the bundle and relocates them in the binary
#

function fixbundle() {
    exec="$1"
    bzroot="$2"
    execpath="$3"
  
    LIBRARIES="`otool -L ${execpath}${exec}.app/Contents/MacOS/${exec} | cut -d' ' -f1`"
    
    for library in $LIBRARIES; do
        
        mkdir -p ${execpath}${exec}.app/Contents/Frameworks
        if [[ "$library" =~ "$2" ]]; then
            echo "${exec}: Migrating `basename $library` in the bundle"
            cp -f $library ${execpath}${exec}.app/Contents/Frameworks
            install_name_tool -change $library @executable_path/../Frameworks/`basename $library` ${execpath}${exec}.app/Contents/MacOS/${exec}
        fi
    done
}


#fixbundle $1 $2 $3

for executable in $EXECUTABLES;
do
   myexecpath="`dirname ${executable}`/"
   myexec="`basename ${executable}|sed -e 's/\.app//'`"
   fixbundle "${myexec}" "`pwd`" "${myexecpath}"
done
