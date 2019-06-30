#!/bin/bash

# author: Maciej Suminski <maciej.suminski@cern.ch>
# contributors: madworm, imcinerney

# Set to 1 to pull the tag given by NGSPICE_GIT_TAG
# Set to 0 to pull the commit with the has given by NGSPICE_GIT_HASH
USE_GIT_TAG=1

NGSPICE_GIT_TAG="ngspice-30-2"
NGSPICE_GIT_HASH="d6f5a32c93a46b6dec8a5097533ddf682cecf2d9"

NGSPICE_GIT="git://git.code.sf.net/p/ngspice/ngspice"

BUILD_DIR="/tmp/libngspice_so"
SRC_DIR="${BUILD_DIR}/ngspice"

if [ -n "${MINGW_PREFIX}" ]; then
    OSTYPE="mingw"
fi

case "${OSTYPE}" in
    "linux"*)
        CFG_OPTIONS="--enable-openmp"
        ;;

    "darwin"*)      # OS X
        # ngspice requires bison 2.7, the one in /usr/bin is 2.3
        export PATH="$(find /usr/local/Cellar/bison -name bin):${PATH}"
        ;;

    "mingw"*)
        CFG_OPTIONS="--prefix ${MINGW_PREFIX} --enable-openmp"
        ;;

    *)
        echo "ERROR: Could not detect the operating system type."
        echo
        echo "Run:"
        echo "OSTYPE=type ${0}"
        echo "where 'type' is linux (Linux), darwin (OSX) or mingw (MinGW)"
        exit 1
        ;;
esac


if [ "$1" = "install" ]; then
    if [ -d ${SRC_DIR} ]; then
	cd ${SRC_DIR}
    else
	echo "*** ngspice has not been built yet"
        exit 1
    fi
    make install

    echo "*** Installation completed successfully! ***"
    exit 0
fi


if [ "$1" = "uninstall" ]; then
    if [ -d ${SRC_DIR} ]; then
	cd ${SRC_DIR}
    else
	echo "*** ngspice has not been built yet"
        exit 1
    fi
    make uninstall

    echo "*** Uninstallation completed successfully! ***"
    exit 0
fi


[ -d "${BUILD_DIR}" ] && rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}" || exit

echo "libngspice (for KiCad) builder v1.2"
echo "(c) CERN 2016"
echo "author: Maciej Suminski <maciej.suminski@cern.ch>"
echo "contributors: madworm, imcinerney"
echo
echo "PREREQUISITES: autoconf automake bison flex gcc git libtool make"
echo

echo "*** Downloading ngspice source code.. ***"
git clone ${NGSPICE_GIT}

if [ $? != 0 ]; then
    echo "*** An error occurred when downloading the source code ***"
    exit 1
fi

if [ -d ${SRC_DIR} ]; then
    cd "${SRC_DIR}"
else
    echo "*** An error occurred when downloading the source code ***"
    exit 1
fi

echo "*** Building libngspice shared library.. ***"
if [ $USE_GIT_TAG == 1 ]; then
    echo "*** Checking out tag ${NGSPICE_GIT_TAG} ***"
    git checkout tags/${NGSPICE_GIT_TAG}
else
    echo "*** Checking out git commit ${NGSPICE_GIT_HASH} ***"
    git checkout ${NGSPICE_GIT_HASH}
fi

./autogen.sh
./configure --with-ngshared --enable-xspice --enable-cider ${CFG_OPTIONS}
make

if [ $? != 0 ]; then
    echo "*** Build failed ***"
    exit 1
fi

echo
echo "*** ngspice shared library has been built successfully! ***"
echo
echo "Now, to finish the installation run the script as root with 'install' parameter:"
echo "sudo $0 install"
echo
echo "It can be uninstalled with 'uninstall' parameter:"
echo "sudo $0 uninstall"
