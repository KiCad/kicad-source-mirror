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

# Downloads and builds LIBWXPYTHON

#-----<configure>----------------------------------------------------------------

set( LIBWXPYTHON_RELEASE 3.0.0.0 )
set( LIBWXPYTHON_MD5 f5e32c7d85dc261ba777e113c3b7e365 )   # re-calc this on every RELEASE change

set( LIBWXPYTHON_ROOT "${PROJECT_SOURCE_DIR}/libwxpython_root" )

#-----</configure>---------------------------------------------------------------

find_package( BZip2 REQUIRED )

set( PREFIX ${DOWNLOAD_DIR}/libwxpython )
set( LIBWXPYTHON_EXEC python )
set( LIBWXPYTHON_OPTS --wxpy_installdir=${LIBWXPYTHON_ROOT}/wxPython )

if (APPLE) 
    SET( LIBWXPYTHON_EXEC python2.6 )
    SET( LIBWXPYTHON_OPTS ${LIBWXPYTHON_OPTS} --osx_cocoa )
    #SET( LIBWXPYTHON_OPTS ${LIBWXPYTHON_OPTS} --mac_framework --mac_framework_prefix=${LIBWXPYTHON_ROOT}/wxPython )

    if( CMAKE_OSX_ARCHITECTURES )
        STRING(REGEX REPLACE " -arch " "," LIBWXPYTHON_ARCHITECTURES ${CMAKE_OSX_ARCHITECTURES})
        SET( LIBWXPYTHON_OPTS ${LIBWXPYTHON_OPTS} --mac_arch=${LIBWXPYTHON_ARCHITECTURES})
    endif( CMAKE_OSX_ARCHITECTURES )

    if( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
        SET( LIBWXPYTHON_PRECMD export CFLAGS=-Qunused-arguments && )
    endif()
endif(APPLE)

if ( KICAD_BUILD_STATIC )
    #message fail
    set( LIBWXPYTHON_BUILDTYPE "--disable-shared" )
endif( KICAD_BUILD_STATIC )

# <SOURCE_DIR> = ${PREFIX}/src/libwx
# There is a Bazaar 'boost scratch repo' in <SOURCE_DIR>/boost and after committing pristine
# download, the patch is applied.  This lets you regenerate a new patch at any time
# easily, simply by editing the working tree in <SOURCE_DIR> and doing "bzr diff" in there.

ExternalProject_Add( libwxpython
    PREFIX          "${PREFIX}"
    DOWNLOAD_DIR    "${DOWNLOAD_DIR}"
    URL             http://sourceforge.net/projects/wxpython/files/wxPython/${LIBWXPYTHON_RELEASE}/wxPython-src-${LIBWXPYTHON_RELEASE}.tar.bz2
    URL_MD5         ${LIBWXPYTHON_MD5}
    STAMP_DIR       "${PREFIX}"

    BUILD_IN_SOURCE 1

    PATCH_COMMAND   bzr revert
        COMMAND     bzr patch -p0 "${PROJECT_SOURCE_DIR}/patches/wxpython-3.0.0_macosx.patch"
        COMMAND     bzr patch -p0 "${PROJECT_SOURCE_DIR}/patches/wxpython-3.0.0_macosx_multiarch.patch" # http://trac.wxwidgets.org/ticket/15957

    UPDATE_COMMAND  ${CMAKE_COMMAND} -E remove_directory "${LIBWXPYTHON_ROOT}"
           COMMAND  ${LIBWXPYTHON_EXEC} wxPython/build-wxpython.py --clean

    CONFIGURE_COMMAND  ${LIBWXPYTHON_PRECMD} ${LIBWXPYTHON_EXEC} wxPython/build-wxpython.py --prefix=${LIBWXPYTHON_ROOT} --unicode --install ${LIBWXPYTHON_OPTS}

    #BINARY_DIR     "${PREFIX}"

    BUILD_COMMAND    true

    INSTALL_DIR     "${LIBWXPYTHON_ROOT}"
    INSTALL_COMMAND   true
    )

ExternalProject_Add_Step( libwxpython bzr_commit_libwxpython
    COMMAND bzr ci -q -m pristine <SOURCE_DIR>
    COMMENT "committing pristine libwxpython files to 'libwxpython scratch repo'"
    DEPENDERS patch
    )


ExternalProject_Add_Step( libwxpython bzr_add_libwxpython
    COMMAND bzr add -q ${PREFIX}/src/libwxpython
    COMMENT "adding pristine libwxpython files to 'libwxpython scratch repo'"
    DEPENDERS bzr_commit_libwxpython
    )


ExternalProject_Add_Step( libwxpython bzr_init_libwxpython
    COMMAND bzr init -q <SOURCE_DIR>
    COMMENT "creating 'libwxpython scratch repo' specifically for libwx to track libwx patches"
    DEPENDERS bzr_add_libwxpython
    DEPENDEES download
    )

######
# Now is time to search what we have built
######

ExternalProject_Add_Step( libwxpython libwxpython_recursive_message
    COMMAND cmake . 
    COMMENT "*** RERUN CMAKE - wxWidgets built, now reissue a cmake to build Kicad"
    DEPENDEES install
    )

