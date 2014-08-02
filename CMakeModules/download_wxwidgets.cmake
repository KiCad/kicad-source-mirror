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

# Downloads and builds LIBWX

#-----<configure>----------------------------------------------------------------

set( LIBWX_RELEASE 3.0.0 )
set( LIBWX_MD5 241998efc12205172ed24c18788ea2cd )   # re-calc this on every RELEASE change

# The boost headers [and static libs if built] go here, at the top of KiCad
# source tree in boost_root.
set( LIBWX_ROOT "${PROJECT_SOURCE_DIR}/libwx_root" )

#-----</configure>---------------------------------------------------------------

if( NOT BZIP2_FOUND )
    find_package( BZip2 REQUIRED )
endif()

set( PREFIX ${DOWNLOAD_DIR}/libwx )

if (APPLE)
    if( CMAKE_OSX_ARCHITECTURES )
        STRING(REGEX REPLACE " -arch " "," LIBWX_ARCHITECTURES ${CMAKE_OSX_ARCHITECTURES})
        SET( LIBWX_ARCHITECTURES --enable-universal_binary=${LIBWX_ARCHITECTURES})
    endif( CMAKE_OSX_ARCHITECTURES )
endif(APPLE)

if ( KICAD_BUILD_STATIC )
    set( LIBWX_BUILDTYPE "--disable-shared" )
endif( KICAD_BUILD_STATIC )

# <SOURCE_DIR> = ${PREFIX}/src/libwx
# There is a Bazaar 'boost scratch repo' in <SOURCE_DIR>/boost and after committing pristine
# download, the patch is applied.  This lets you regenerate a new patch at any time
# easily, simply by editing the working tree in <SOURCE_DIR> and doing "bzr diff" in there.

ExternalProject_Add( libwx
    PREFIX          "${PREFIX}"
    DOWNLOAD_DIR    "${DOWNLOAD_DIR}"
    URL             http://downloads.sourceforge.net/project/wxwindows/${LIBWX_RELEASE}/wxWidgets-${LIBWX_RELEASE}.tar.bz2
    URL_MD5         ${LIBWX_MD5}
    STAMP_DIR       "${PREFIX}"

    BUILD_IN_SOURCE 1

    PATCH_COMMAND   bzr revert
        COMMAND     bzr patch -p0 "${PROJECT_SOURCE_DIR}/patches/wxwidgets-3.0.0_macosx.patch"
        COMMAND     bzr patch -p0 "${PROJECT_SOURCE_DIR}/patches/wxwidgets-3.0.0_macosx_bug_15908.patch"

    UPDATE_COMMAND  ${CMAKE_COMMAND} -E remove_directory "${LIBWX_ROOT}"

    CONFIGURE_COMMAND  ./configure --prefix=${LIBWX_ROOT} -with-opengl --enable-aui --enable-debug_info -with-expat=builtin --with-regex=builtin --enable-utf8 ${LIBWX_ARCHITECTURES} ${LIBWX_BUILDTYPE}
    #BINARY_DIR     "${PREFIX}"

    BUILD_COMMAND    $(MAKE) VERBOSE=1

    INSTALL_DIR     "${LIBWX_ROOT}"
    INSTALL_COMMAND  make install
    )

#SET directories
set(wxWidgets_BIN_DIR           ${LIBWX_ROOT}/bin)
set(wxWidgets_CONFIG_EXECUTABLE ${LIBWX_ROOT}/bin/wx-config)
set(wxWidgets_INCLUDE_DIRS      ${LIBWX_ROOT}/include)
set(wxWidgets_LIBRARY_DIRS      ${LIBWX_ROOT}/lib)


ExternalProject_Add_Step( libwx bzr_commit_libwx
    COMMAND bzr ci -q -m pristine <SOURCE_DIR>
    COMMENT "committing pristine libwx files to 'libwx scratch repo'"
    DEPENDERS patch
    )


ExternalProject_Add_Step( libwx bzr_add_libwx
    COMMAND bzr add -q ${PREFIX}/src/libwx
    COMMENT "adding pristine libwx files to 'libwx scratch repo'"
    DEPENDERS bzr_commit_libwx
    )


ExternalProject_Add_Step( libwx bzr_init_libwx
    COMMAND bzr init -q <SOURCE_DIR>
    COMMENT "creating 'libwx scratch repo' specifically for libwx to track libwx patches"
    DEPENDERS bzr_add_libwx
    DEPENDEES download
    )

######
# Now is time to search what we have built
######

ExternalProject_Add_Step( libwx libwx_recursive_message
    COMMAND cmake .
    COMMENT "*** RERUN CMAKE - wxWidgets built, now reissue a cmake to build Kicad"
    DEPENDEES install
    )

