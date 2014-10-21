#!/bin/bash -e
# Install KiCad from source onto either:
#  -> a Ubuntu/Debian/Mint or
#  -> a Red Hat
# compatible linux system.
#
# The "install_prerequisites" step is the only "distro dependent" one.  That step could be modified
# for other linux distros.
#
# There are 3 package groups in a KiCad install:
# 1) Compiled source code in the form of executable programs.
# 2) User manuals and other documentation typically as *.pdf files.
# 3) a) Schematic parts, b) layout footprints, and c) 3D models for footprints.
#
# To achieve 1) source is checked out from its repo and compiled by this script then executables
#  are installed using CMake.
# To achieve 2) documentation is checked out from its repo and installed using CMake.
# TO achieve 3a) and 3c) they are checked out from their repos and installed using CMake.
# To achieve 3b) a global fp-lib-table is put into your home directory which points to
#  http://github.com/KiCad.  No actual footprints are installed locally, internet access is used
#  during program operation to fetch footprints from github as if it was a remote drive in the cloud.
#  If you want to install those same KiCad footprints locally, you may run a separate script
#  named library-repos-install.sh found in this same directory.  That script requires that "git" be on
#  your system whereas this script does not.  The footprints require some means to download them and
#  bzr-git seems not up to the task.  wget or curl would also work.


# Since bash is invoked with -e by the first line of this script, all the steps in this script
# must succeed otherwise bash will abort at the first non-zero error code.  Therefore any script
# functions must be crafted to anticipate numerous conditions, such that no command fails unless it
# is a serious situation.


# Set where the 3 source trees will go, use a full path
WORKING_TREES=~/kicad_sources

STABLE=5054             # a sensible mix of features and stability
TESTING=last:1          # the most recent

# Set this to STABLE or TESTING or other known revision number:
REVISION=$TESTING

# For info on revision syntax:
# $ bzr help revisionspec


# CMake Options
OPTS="$OPTS -DBUILD_GITHUB_PLUGIN=ON"       # needed by $STABLE revision

# Python scripting, uncomment to enable
#OPTS="$OPTS -DKICAD_SCRIPTING=ON -DKICAD_SCRIPTING_MODULES=ON -DKICAD_SCRIPTING_WXPYTHON=ON"

# Use https under bazaar to retrieve repos because this does not require a
# launchpad.net account.  Whereas lp:<something> requires a launchpad account.
# https results in read only access.
REPOS=https://code.launchpad.net

# This branch is a bzr/launchpad import of the Git repository
# at https://github.com/KiCad/kicad-library.git.
# It has schematic parts and 3D models in it.
LIBS_REPO=$REPOS/~kicad-product-committers/kicad/library

SRCS_REPO=$REPOS/~kicad-product-committers/kicad/product
DOCS_REPO=$REPOS/~kicad-developers/kicad/doc


usage()
{
    echo ""
    echo " usage:"
    echo ""
    echo "./kicad-install.sh <cmd>"
    echo "    where <cmd> is one of:"
    echo "      --install-or-update     (does full installation or update.)"
    echo "      --remove-sources        (removes source trees for another attempt.)"
    echo "      --uninstall-libraries   (removes KiCad supplied libraries.)"
    echo "      --uninstall-kicad       (uninstalls all of KiCad but leaves source trees.)"
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
        prerequisite_list="
            bzr
            bzrtools
            build-essential
            cmake
            cmake-curses-gui
            debhelper
            doxygen
            grep
            libbz2-dev
            libcairo2-dev
            libglew-dev
            libssl-dev
            libwxgtk3.0-dev
       "

        for p in ${prerequisite_list}
        do
            sudo apt-get install $p || exit 1
        done

        # Only install the scripting prerequisites if required.
        if [ "$(expr match "$OPTS" '.*\(-DKICAD_SCRIPTING=ON\)')" == "-DKICAD_SCRIPTING=ON" ]; then
        #echo "KICAD_SCRIPTING=ON"
            scripting_prerequisites="
                python-dev
                python-wxgtk3.0-dev
                swig
            "

            for sp in ${scripting_prerequisites}
            do
                sudo apt-get install $sp || exit 1
            done
        fi

    # assume all yum systems have same prerequisites
    elif [ "$(expr match "$PM" '.*\(yum\)')" == "yum" ]; then
        #echo "red hat compatible system"
        # Note: if you find this list not to be accurate, please submit a patch:
        sudo yum groupinstall "Development Tools" || exit 1

        prerequisite_list="
            install
            bzr
            bzrtools
            bzip2-libs
            bzip2-devel
            cmake
            cmake-gui
            doxygen
            cairo-devel
            glew-devel
            grep
            openssl-devel
            wxGTK-devel
        "

        for p in ${prerequisite_list}
        do
            sudo yum install $p || exit 1
        done

        # Only install the scripting prerequisites if required.
        if [ "$(expr match "$OPTS" '.*\(-DKICAD_SCRIPTING=ON\)')" == "-DKICAD_SCRIPTING=ON" ]; then
        #echo "KICAD_SCRIPTING=ON"
            scripting_prerequisites="
                swig
                wxPython
            "

            for sp in ${scripting_prerequisites}
            do
                sudo yum install $sp || exit 1
            done
        fi
    else
        echo
        echo "Incompatible System. Neither 'yum' nor 'apt-get' found. Not possible to continue."
        echo
        exit 1
    fi

    # ensure bzr name and email are set.  No message since bzr prints an excellent diagnostic.
    bzr whoami || {
        echo "WARNING: You have not set bzr whoami, so I will set a dummy."
        export BZR_EMAIL="Kicad Build <nobody@foo>"
    }
}


rm_build_dir()
{
    local dir="$1"

    echo "removing directory $dir"

    if [ -e "$dir/install_manifest.txt" ]; then
        # this file is often created as root, so remove as root
        sudo rm "$dir/install_manifest.txt" 2> /dev/null
    fi

    if [ -d "$dir" ]; then
        rm -rf "$dir"
    fi
}


cmake_uninstall()
{
    # assume caller set the CWD, and is only telling us about it in $1
    local dir="$1"

    cwd=`pwd`
    if [ "$cwd" != "$dir" ]; then
        echo "missing dir $dir"
    elif [ ! -e install_manifest.txt  ]; then
        echo
        echo "Missing file $dir/install_manifest.txt."
    else
        echo "uninstalling from $dir"
        sudo make uninstall
        sudo rm install_manifest.txt
    fi
}


# Function set_env_var
# sets an environment variable globally.
set_env_var()
{
    local var=$1
    local val=$2

    if [ -d /etc/profile.d ]; then
        if [ ! -e /etc/profile.d/kicad.sh ] || ! grep "$var" /etc/profile.d/kicad.sh >> /dev/null; then
            echo
            echo "Adding environment variable $var to file /etc/profile.d/kicad.sh"
            echo "Please logout and back in after this script completes for environment"
            echo "variable to get set into environment."
            sudo sh -c "echo export $var=$val >> /etc/profile.d/kicad.sh"
        fi

    elif [ -e /etc/environment ]; then
        if ! grep "$var" /etc/environment >> /dev/null; then
            echo
            echo "Adding environment variable $var to file /etc/environment"
            echo "Please reboot after this script completes for environment variable to get set into environment."
            sudo sh -c "echo $var=$val >> /etc/environment"
        fi
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
        bzr checkout -r $REVISION $SRCS_REPO kicad.bzr
        echo " source repo to local working tree."
    else
        cd kicad.bzr
        bzr up -r $REVISION
        echo " local source working tree updated."
        cd ../
    fi

    echo "step 4) checking out the schematic parts and 3D library repo."
    if [ ! -d "$WORKING_TREES/kicad-lib.bzr" ]; then
        bzr checkout $LIBS_REPO kicad-lib.bzr
        echo ' kicad-lib checked out.'
    else
        cd kicad-lib.bzr
        bzr up
        echo ' kicad-lib repo updated.'
        cd ../
    fi

    echo "step 5) checking out the documentation from launchpad repo..."
    if [ ! -d "$WORKING_TREES/kicad-doc.bzr" ]; then
        bzr checkout $DOCS_REPO kicad-doc.bzr
        echo " docs checked out."
    else
        cd kicad-doc.bzr
        bzr up
        echo " docs working tree updated."
        cd ../
    fi


    echo "step 6) compiling source code..."
    cd kicad.bzr
    if [ ! -d "build" ]; then
        mkdir build && cd build
        cmake $OPTS ../ || exit 1
    else
        cd build
        # Although a "make clean" is sometimes needed, more often than not it slows down the update
        # more than it is worth.  Do it manually if you need to in this directory.
        # make clean
    fi
    make -j4 || exit 1
    echo " kicad compiled."


    echo "step 7) installing KiCad program files..."
    sudo make install
    echo " kicad program files installed."


    echo "step 8) installing libraries..."
    cd ../../kicad-lib.bzr
    rm_build_dir build
    mkdir build && cd build
    cmake ../
    sudo make install
    echo " kicad-lib.bzr installed."


    echo "step 9) as non-root, install global fp-lib-table if none already installed..."
    # install ~/fp-lib-table
    if [ ! -e ~/fp-lib-table ]; then
        make  install_github_fp-lib-table
        echo " global fp-lib-table installed."
    fi


    echo "step 10) installing documentation..."
    cd ../../kicad-doc.bzr
    rm_build_dir build
    mkdir build && cd build
    cmake ../
    sudo make install
    echo " kicad-doc.bzr installed."

    echo "step 11) check for environment variables..."
    if [ -z "${KIGITHUB}" ]; then
        set_env_var KIGITHUB https://github.com/KiCad
    fi

    echo
    echo 'All KiCad "--install-or-update" steps completed, you are up to date.'
    echo
}


if [ $# -eq 1 -a "$1" == "--remove-sources" ]; then
    echo "deleting $WORKING_TREES"
    rm_build_dir "$WORKING_TREES/kicad.bzr/build"
    rm_build_dir "$WORKING_TREES/kicad-lib.bzr/build"
    rm_build_dir "$WORKING_TREES/kicad-doc.bzr/build"
    rm -rf "$WORKING_TREES"
    exit
fi


if [ $# -eq 1 -a "$1" == "--install-or-update" ]; then
    install_or_update
    exit
fi


if [ $# -eq 1 -a "$1" == "--uninstall-libraries" ]; then
    cd "$WORKING_TREES/kicad-lib.bzr/build"
    cmake_uninstall "$WORKING_TREES/kicad-lib.bzr/build"
    exit
fi


if [ $# -eq 1 -a "$1" == "--uninstall-kicad" ]; then
    cd "$WORKING_TREES/kicad.bzr/build"
    cmake_uninstall "$WORKING_TREES/kicad.bzr/build"

    cd "$WORKING_TREES/kicad-lib.bzr/build"
    cmake_uninstall "$WORKING_TREES/kicad-lib.bzr/build"

    # this may fail since "uninstall" support is a recent feature of this repo:
    cd "$WORKING_TREES/kicad-doc.bzr/build"
    cmake_uninstall "$WORKING_TREES/kicad-doc.bzr/build"

    exit
fi


usage
