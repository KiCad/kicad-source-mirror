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
set( OPENSSL_MD5 66bf6f10f060d561929de96f9dfe5b8c ) # re-calc on every RELEASE change

#-----</configure>-----------------------------------------------------------------------------------

unset( PIC_FLAG )
set( CFLAGS CFLAGS=${CMAKE_C_FLAGS} )

if( MINGW )         # either MINGW or cross compiling?
    if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
        set( MINGW32 true )
        set( MACHINE x86_32 )
    elseif( CMAKE_SIZEOF_VOID_P EQUAL 8 )
        set( MINGW64 true )
        set( MACHINE x86_64 )
    endif()

    if( MINGW32 )
        set( HOST "--host=i586-pc-mingw32" )
    elseif( MINGW64 )
        set( HOST "--host=x86_64-pc-mingw32" )
    endif()

    set( CC "CC=${CMAKE_C_COMPILER}" )
    set( RANLIB "RANLIB=${CMAKE_RANLIB}" )
    set( AR "AR=${CMAKE_AR}" )
else()
    set( PIC_FLAG -fPIC )
endif()

string( TOLOWER ${CMAKE_HOST_SYSTEM_NAME} build )

# Force some configure scripts into knowing this is a cross-compilation, if it is.
set( BUILD --build=${CMAKE_HOST_SYSTEM_PROCESSOR}-pc-${build} )


# http://www.blogcompiler.com/2011/12/21/openssl-for-windows/
# http://qt-project.org/wiki/Compiling-OpenSSL-with-MinGW
set( PREFIX ${DOWNLOAD_DIR}/openssl-${OPENSSL_RELEASE} )

unset( CROSS )
if( MINGW32 )
    set( MW mingw )
    set( CROSS "CROSS_COMPILE=${CROSS_COMPILE}" )
elseif( MINGW64 )
    set( MW mingw64 )
    set( CROSS "CROSS_COMPILE=${CROSS_COMPILE}" )
endif()

ExternalProject_Add(
    openssl
    DOWNLOAD_DIR ${DOWNLOAD_DIR}
    PREFIX ${PREFIX}
    TIMEOUT 60
    URL http://www.openssl.org/source/openssl-${OPENSSL_RELEASE}.tar.gz
    URL_MD5 ${OPENSSL_MD5}

    # mingw uses msvcrt.dll's printf() which cannot handle %zd, so having
    # BIO_snprintf() reference printf()'s formating attributes is a bug, since
    # BIO_snprintf() does its own formatting and is different from msvcrt's printf().

    # This one would be easier if Windows folks could be asked to install "patch.exe"
    # PATCH_COMMAND patch -p0 < ${PROJECT_SOURCE_DIR}/patches/openssl-1.0.1e.patch

    # This one requires the bzr commit below, since bzr patch only knows a working tree.
    PATCH_COMMAND bzr patch -p0 ${PROJECT_SOURCE_DIR}/patches/openssl-1.0.1e.patch

    # this requires that perl be installed:
    CONFIGURE_COMMAND
            ${CROSS}
            <SOURCE_DIR>/Configure
            ${MW}
            --prefix=<INSTALL_DIR>
            ${PIC_FLAG}         # empty for MINGW
            shared

    BUILD_IN_SOURCE 1
    BUILD_COMMAND make depend
          COMMAND make
    INSTALL_COMMAND make install
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
    ${PREFIX}/lib/libssl.a
    ${PREFIX}/lib/libcrypto.a
    CACHE STRING "OPENSSL libraries"
    )
set( OPENSSL_FOUND true )

