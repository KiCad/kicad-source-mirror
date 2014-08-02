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

# Downloads and builds CAIRO

#-----<configure>----------------------------------------------------------------

set( PKGCONFIG_RELEASE 0.28 )
set( PKGCONFIG_MD5 aa3c86e67551adc3ac865160e34a2a0d )   # re-calc this on every RELEASE change

# The boost headers [and static libs if built] go here, at the top of KiCad
# source tree in boost_root.
set( PKGCONFIG_ROOT "${PROJECT_SOURCE_DIR}/pkgconfig_root" )

#-----</configure>---------------------------------------------------------------

if( NOT BZIP2_FOUND )
    find_package( BZip2 REQUIRED )
endif()

set( PREFIX ${DOWNLOAD_DIR}/pkgconfig )

if ( KICAD_BUILD_STATIC )
    set( PKGCONFIG_BUILDTYPE --enable-shared=no --enable-static=yes )
else()
    set( PKGCONFIG_BUILDTYPE --enable-shared=yes --enable-static=yes )
endif( KICAD_BUILD_STATIC )

# <SOURCE_DIR> = ${PREFIX}/src/glew
# There is a Bazaar 'boost scratch repo' in <SOURCE_DIR>/boost and after committing pristine
# download, the patch is applied.  This lets you regenerate a new patch at any time
# easily, simply by editing the working tree in <SOURCE_DIR> and doing "bzr diff" in there.

ExternalProject_Add( pkgconfig
    PREFIX          "${PREFIX}"
    DOWNLOAD_DIR    "${DOWNLOAD_DIR}"
    URL             http://pkgconfig.freedesktop.org/releases/pkg-config-${PKGCONFIG_RELEASE}.tar.gz
    URL_MD5         ${PKGCONFIG_MD5}
    STAMP_DIR       "${PREFIX}"

    #SOURCE_DIR      "${PREFIX}"
    BUILD_IN_SOURCE 1

    #PATCH_COMMAND     "true"
    CONFIGURE_COMMAND  ./configure --prefix=${PKGCONFIG_ROOT} --with-internal-glib ${PKGCONFIG_BUILDTYPE} --disable-silent-rules
    #BINARY_DIR      "${PREFIX}"

    UPDATE_COMMAND  ${CMAKE_COMMAND} -E remove_directory "${PKGCONFIG_ROOT}"

    BUILD_COMMAND   $(MAKE)

    INSTALL_DIR     "${PKGCONFIG_ROOT}"
    INSTALL_COMMAND $(MAKE) install
    )

ExternalProject_Add_Step( pkgconfig pkgconfig_cleanup
    COMMAND  ${CMAKE_COMMAND} -E remove_directory "${PKGCONFIG_ROOT}"
    COMMENT "pkgconfig - cleanup destination before proceeding in install"
    DEPENDEES build
    )

