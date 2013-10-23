#!/bin/bash
# Install KiCad from source onto either:
#  -> a Ubuntu/Debian/Mint or
#  -> a Red Hat
# compatible linux system.
#
# The "install_prerequisites" step is the only "distro dependent" one.  Could modify
# that step for other linux distros.


# Set where the 3 source trees will go
WORKING_TREES=~/kicad_sources


usage()
{
    echo ""
    echo " usage:"
    echo ""
    echo "./kicad-install.sh <cmd>"
    echo "    where <cmd> is one of:"
    echo "        --install-or-update  (does full installation or update.)"
    echo "        --remove-sources     (removes source trees for another attempt.)"
    echo ""
    echo "example:"
    echo '    $ ./kicad-install.sh --install-or-update'
}


install_prerequisites()
{
    # Find a package manager, PM
    PM=$( command -v yum || command -v apt-get )

    # assume all these Debian, Mint, Ubuntu systems have same prerequisites
    if [ "$(expr match "$PM" '.*\(apt-get\)')" == "apt-get" ]; then
        #echo "debian compatible system"
        sudo apt-get install \
            bzr \
            bzrtools \
            build-essential \
            cmake \
            cmake-curses-gui \
            debhelper \
            doxygen \
            libbz2-dev \
            libssl-dev \
            libwxgtk2.8-dev

    # assume all yum systems have same prerequisites
    elif [ "$(expr match "$PM" '.*\(yum\)')" == "yum" ]; then
        #echo "red hat compatible system"
        # Note: if you find this list not to be accurate, please submit a patch:
        sudo yum install
            bzr \
            bzrtools \
            build-essential \
            cmake \
            cmake-curses-gui \
            debhelper \
            doxygen \
            libbz2-dev \
            libgl1-mesa-dev \
            libglu1-mesa-dev \
            libssl-dev \
            libwxbase2.8-dev \
            libwxgtk2.8-dev \
            libx11-dev \
            mesa-common-dev
    else
        echo
        echo "Incompatible System. Neither 'yum' nor 'apt-get' found. Not possible to continue."
        echo
        exit 1
    fi
}


install_or_update()
{
    echo "step 1) installing pre-requisites"
    install_prerequisites


    echo "step 2) make $WORKING_TREES if it does not exist"
    if [ ! -d "$WORKING_TREES" ]; then
        sudo mkdir -p "$WORKING_TREES"
        echo " mark $WORKING_TREES as owned by me"
        sudo chown -R `whoami` "$WORKING_TREES"
    fi
    cd $WORKING_TREES


    echo "step 3) checking out the source code from launchpad repo..."
    if [ ! -d "$WORKING_TREES/kicad.bzr" ]; then
        bzr checkout lp:kicad kicad.bzr
        echo " source repo to local working tree."
    else
        cd kicad.bzr
        bzr up
        echo " local source working tree updated."
        cd ../
    fi


    echo "step 4) checking out the libraries from launchpad repo..."
    if [ ! -d "$WORKING_TREES/kicad-lib.bzr" ]; then
        bzr checkout lp:~kicad-lib-committers/kicad/library kicad-lib.bzr
        echo ' kicad-lib just checked out.'
    else
        cd kicad-lib.bzr
        bzr up
        echo ' kicad-lib repo just updated.'
        cd ../
    fi

    echo "step 5) checking out the documentation from launchpad repo..."
    if [ ! -d "$WORKING_TREES/kicad-doc.bzr" ]; then
        bzr checkout lp:~kicad-developers/kicad/doc kicad-doc.bzr
        echo " docs checked out."
    else
        cd kicad-doc.bzr
        bzr up
        echo " docs working tree updated."
        cd ../
    fi


    echo "step 6) compiling source code..."
    cd kicad.bzr
    rm -rf build && mkdir build && cd build
    cmake   -DCMAKE_BUILD_TYPE=Release \
            -DUSE_FP_LIB_TABLE=ON \
            -DBUILD_GITHUB_PLUGIN=ON \
            ../
    make
    echo " kicad compiled."


    echo "step 7) installing KiCad program files..."
    sudo make install
    echo " kicad program files installed."


    echo "step 8) as non-root, install user configuration files..."
    # install ~/fp-lib-table [and friends]
    make install_user_configuration_files
    echo " kicad user-configuration files installed."


    echo "step 9) installing libraries..."
    cd ../../kicad-lib.bzr
    rm -rf build && mkdir build && cd build
    cmake ../
    sudo make install
    echo " kicad-lib installed."


    echo "step 10) installing documentation..."
    cd ../../kicad-doc.bzr
    rm -rf build && mkdir build && cd build
    cmake ../
    sudo make install
    echo " kicad-doc.bzr installed."
}


if [ $# -eq 1 -a "$1" == "--remove-sources" ]; then
    # run this only once, kills .config & makes dirs
    echo "deleting $WORKING_TREES"
    rm -rf "$WORKING_TREES"
    exit
fi


if [ $# -eq 1 -a "$1" == "--install-or-update" ]; then
    install_or_update
    exit
fi

usage
