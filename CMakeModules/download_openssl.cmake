#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
#  Copyright (C) 2013 Kicad Developers, see AUTHORS.txt for contributors.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, you may find one here:
#  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
#  or you may search the http://www.gnu.org website for the version 2 license,
#  or you may write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA



# Download OPENSSL and install into ${PREFIX}, typically in our KiCad source tree.
# Assumes include( ExternalProject ) was done inline previous to this file
# and that set( DOWNLOAD_DIR ... ) was set in a higher context.

#-----<configure>-------------------------------------------------------------------------------------

set( OPENSSL_RELEASE "1.0.1e" )
set( OPENSSL_MD5 08bec482fe1c4795e819bfcfcb9647b9 ) # re-calc on every RELEASE change

#-----</configure>-----------------------------------------------------------------------------------

set( PREFIX ${DOWNLOAD_DIR}/openssl-${OPENSSL_RELEASE} )

# CMake barfs if we pass in an empty CMAKE_TOOLCHAIN_FILE, so we only set it up
# if it has a non-empty value
unset( TOOLCHAIN )
if( CMAKE_TOOLCHAIN_FILE )
    set( TOOLCHAIN "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}" )
endif()

FIND_PROGRAM (patch_bin NAMES patch.exe patch)

if( "${patch_bin}" STREQUAL "patch_bin-NOTFOUND" )
    set( PATCH_STR_CMD bzr patch -p0 )
else()
    set( PATCH_STR_CMD ${patch_bin} -p0 -i )
endif()

ExternalProject_Add(
    openssl
    DOWNLOAD_DIR ${DOWNLOAD_DIR}
    PREFIX ${PREFIX}
    TIMEOUT 60
    URL http://launchpad.net/openssl-cmake/1.0.1e/1.0.1e-1/+download/openssl-cmake-1.0.1e-src.tar.gz
    URL_MD5 ${OPENSSL_MD5}

    # mingw uses msvcrt.dll's printf() which cannot handle %zd, so having
    # BIO_snprintf() reference printf()'s formating attributes is a bug, since
    # BIO_snprintf() does its own formatting and is different from msvcrt's printf().

    # This one would be easier if Windows folks could be asked to install "patch.exe"
    # PATCH_COMMAND patch -p0 < ${PROJECT_SOURCE_DIR}/patches/openssl-1.0.1e.patch

    # This one requires the bzr commit below, since bzr patch only knows a working tree.

    # Revert the branch to pristine before applying patch sets as bzr patch
    # fails when applying a patch to the branch twice and doesn't have a switch
    # to ignore previously applied patches
    PATCH_COMMAND   bzr revert
        # PATCH_COMMAND continuation (any *_COMMAND here can be continued with COMMAND):
        COMMAND    ${PATCH_STR_CMD} ${PROJECT_SOURCE_DIR}/patches/openssl-1.0.1e.patch

    CONFIGURE_COMMAND
            ${CMAKE_COMMAND}
            -G ${CMAKE_GENERATOR}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DBUILD_SHARED_LIBS=ON
            ${TOOLCHAIN}
            <SOURCE_DIR>

    BUILD_IN_SOURCE 1
    BUILD_COMMAND ${CMAKE_MAKE_PROGRAM}
    INSTALL_COMMAND ${CMAKE_MAKE_PROGRAM} install
    )

# In order to use "bzr patch", we have to have a bzr working tree, this means a bzr repo
# must be created and source committed to it.  These extra steps do that.

set( target "openssl" )

ExternalProject_Add_Step( ${target} bzr_commit_${target}
    COMMAND bzr ci -q -m pristine <SOURCE_DIR>
    COMMENT "committing pristine ${target} files to '${target} scratch repo'"
    DEPENDERS patch
    )

ExternalProject_Add_Step( ${target} bzr_add_${target}
    COMMAND bzr add -q <SOURCE_DIR>
    COMMENT "adding pristine ${target} files to '${target} scratch repo'"
    DEPENDERS bzr_commit_${target}
    )

ExternalProject_Add_Step( ${target} bzr_init_${target}
    COMMAND bzr init -q <SOURCE_DIR>
    COMMENT "creating '${target} scratch repo' specifically for tracking ${target} patches"
    DEPENDERS bzr_add_${target}
    DEPENDEES download
    )

# The spelling of these is always taken from CMake Module's FindXYZ.cmake file:
set( OPENSSL_INCLUDE_DIR
    ${PREFIX}/include
    CACHE FILEPATH "OPENSSL include directory"
    )

set( OPENSSL_LIBRARIES
    ${PREFIX}/lib/libssl${CMAKE_IMPORT_LIBRARY_SUFFIX}
    ${PREFIX}/lib/libcrypto${CMAKE_IMPORT_LIBRARY_SUFFIX}
    CACHE STRING "OPENSSL libraries"
    )
set( OPENSSL_FOUND true )

