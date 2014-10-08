#!/bin/bash
#
# Small helper script for patching/compiling wxWidgets/wxPython on OSX
#
# Params
#   $1  wxWidgets/wxPython source folder (relative to current dir)
#   $2  Target bin folder
#   $3  KiCad source folder (relative to current dir)
#   $4  Make options (e.g., "-j4")

createPaths() {
    echo "*** Creating/wiping build and bin folder..."

    rm -rf wx-build
    rm -rf $1
    mkdir wx-build
    mkdir $1
}

doPatch() {
    cwd=$(pwd)
    cd $1

    patchcmd="patch -p0 -RN --dry-run < $cwd/$2"
    eval $patchcmd &> /dev/null
    if [ $? -eq 0 ];
    then
        echo "*** Patch '$2' already applied, skipping..."
    else
        echo "*** Applying patch '$2'..."

        patch -p0 < $cwd/$2
        if [ $? -ne 0 ];
        then
            cd $cwd
            exit 1
        fi
    fi

    cd $cwd
}

wxWidgets_configure() {
    echo "*** Configuring wxWidgets..."
    cwd=$(pwd)
    cd wx-build

    ../$1/configure \
        --prefix=$cwd/$2 \
        --with-opengl \
        --enable-aui \
        --enable-utf8 \
        --enable-html \
        --enable-stl \
        --with-libjpeg=builtin \
        --with-libpng=builtin \
        --with-regex=builtin \
        --with-libtiff=builtin \
        --with-zlib=builtin \
        --with-expat=builtin \
        --without-liblzma \
        --with-macosx-version-min=10.5 \
        --enable-universal-binary=i386,x86_64 \
        CPPFLAGS="-stdlib=libstdc++" \
        LDFLAGS="-stdlib=libstdc++" \
        CC=clang \
        CXX=clang++
    if [ $? -ne 0 ];
    then
        cd ..
        exit 1
    fi

    cd ..
}

wxWidgets_buildInst() {
    echo "*** Building wxWidgets..."
    cd wx-build

    make $1 install
    if [ $? -ne 0 ];
    then
        cd ..
        exit 1
    fi

    cd ..
}

wxPython_buildInst() {
    cwd=$(pwd)
    cd $1/wxPython

    # build params
    WXPYTHON_BUILD_OPTS="WX_CONFIG=$cwd/$2/bin/wx-config \
        BUILD_BASE=$cwd/wx-build \
        UNICODE=1 \
        WXPORT=osx_cocoa"

    # build
    python setup.py build_ext $WXPYTHON_BUILD_OPTS
    if [ $? -ne 0 ];
    then
        cd $cwd
        exit 1
    fi

    # install
    python setup.py install --prefix=$cwd/$2 $WXPYTHON_BUILD_OPTS
    if [ $? -ne 0 ];
    then
        cd $cwd
        exit 1
    fi

    cd $cwd
}


# check parameters
if [ "$#" -lt 3 ];
then
    echo "OSX wxWidgets/wxPython build script"
    echo
    echo "Usage:"
    echo "  osx_build_wx.sh <src> <bin> <kicad> <makeopts>"
    echo "    <src>       wxWidgets/wxPython source folder"
    echo "    <bin>       Destination folder"
    echo "    <kicad>     KiCad folder"
    echo "    <makeopts>  Optional: make options for building wxWidgets (e.g., -j4)"
    exit 1
fi

# create build paths
createPaths "$2"

# patch wxWidgets sources
echo "*** Patching wxWidgets..."
doPatch "$1" "$3/patches/wxwidgets-3.0.0_macosx.patch"
doPatch "$1" "$3/patches/wxwidgets-3.0.0_macosx_bug_15908.patch"
doPatch "$1" "$3/patches/wxwidgets-3.0.0_macosx_soname.patch"

# configure and build wxWidgets
wxWidgets_configure "$1" "$2"
wxWidgets_buildInst "$4"

# check if source is wxPython
if [ -d $1/wxPython ];
then
    echo "*** Source is wxPython, now building wxPython stuff..."
    wxPython_buildInst "$1" "$2"
fi

# remove build dir
echo "*** Removing build folder"
rm -rf wx-build

# done
echo "*** Finished building!"


