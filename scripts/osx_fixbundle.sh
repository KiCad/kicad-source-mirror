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
        if [[ "$library" =~ "$bzroot" ]]; then
            echo "${exec}: Migrating `basename $library` in the bundle"
            if [ ! -f ${exec}.app/Contents/Frameworks/`basename $library` ]; then
                cp -f $library ${execpath}${exec}.app/Contents/Frameworks
            fi
            install_name_tool -change $library @executable_path/../Frameworks/`basename $library` ${execpath}${exec}.app/Contents/MacOS/${exec}
        fi
    done

    # Resolve issue in python modules (.so)
    cd ${execpath}
    MODULES="`find ${exec}.app -name '*.so'`"
    
    for module in $MODULES; do
        LIBRARIES="`otool -L $module | cut -d' ' -f1`"
        mkdir -p ${exec}.app/Contents/Frameworks

        for library in $LIBRARIES; do
            if [[ "$library" =~ "$bzroot" ]]; then
                if [ ! -f ${exec}.app/Contents/Frameworks/`basename $library` ]; then
                    cp -f $library ${exec}.app/Contents/Frameworks
                fi
                install_name_tool -change $library @executable_path/../Frameworks/`basename $library` $module
            fi 
        done
        echo "${exec}: elaborated module `basename ${module}`"
    done

    # Resolve issue between DYNLIBS
    dynlib_migrate="1";
    dynlib_cycle="0";
    
    while [ $dynlib_migrate -gt 0 ]; do
        dynlib_migrate="0";
        (( dynlib_cycle += 1 ))
        DYNLIBS="`find ${exec}.app -name '*.dylib'`"
    
        for dynlib in $DYNLIBS; do
            LIBRARIES="`otool -L $dynlib | cut -d' ' -f1`"
            mkdir -p ${exec}.app/Contents/Frameworks
    
            for library in $LIBRARIES; do
                if [[ "$library" =~ "$bzroot" ]]; then
                    if [ ! -f ${exec}.app/Contents/Frameworks/`basename $library` ]; then
                        cp -f $library ${exec}.app/Contents/Frameworks
                        echo "copied `basename $library` into bundle"
                        (( dynlib_migrate += 1))
                    fi

                    install_name_tool -change $library @executable_path/../Frameworks/`basename $library` $dynlib
                fi
            done
        done
        echo "${exec}: bundle dynlib dependencies migration Pass $dynlib_cycle: Migrated $dynlib_migrate libraries in bundle"
    done  
    cd - >/dev/null
}


#fixbundle $1 $2 $3

for executable in $EXECUTABLES;
do
   myexecpath="`dirname ${executable}`/"
   myexec="`basename ${executable}|sed -e 's/\.app//'`"
   fixbundle "${myexec}" "`pwd`" "${myexecpath}"
done
