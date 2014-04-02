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

# Downloads and builds PIXMAN

#-----<configure>----------------------------------------------------------------

set( PIXMAN_RELEASE 0.32.4 )
set( PIXMAN_MD5 eba449138b972fbf4547a8c152fea162 )   # re-calc this on every RELEASE change

# The boost headers [and static libs if built] go here, at the top of KiCad
# source tree in boost_root.
set( PIXMAN_ROOT "${PROJECT_SOURCE_DIR}/pixman_root" )

#-----</configure>---------------------------------------------------------------

if( NOT BZIP2_FOUND )
    find_package( BZip2 REQUIRED )
endif()

set( PREFIX ${DOWNLOAD_DIR}/pixman )

set(PIXMAN_CPPFLAGS "CFLAGS=")

if (APPLE)
    if( CMAKE_OSX_ARCHITECTURES )
        set(PIXMAN_CPPFLAGS "${PIXMAN_CPPFLAGS} -arch ${CMAKE_OSX_ARCHITECTURES} -fno-common -mmacosx-version-min=10.5")
    else()
        set(PIXMAN_CPPFLAGS "${PIXMAN_CPPFLAGS} -fno-common -mmacosx-version-min=10.5")
    endif( CMAKE_OSX_ARCHITECTURES )
endif(APPLE)

if (KICAD_BUILD_STATIC)
    set(PIXMAN_OPTS --enable-static=yes --enable-shared=no)
else()
    set(PIXMAN_OPTS --enable-static=yes --enable-shared=yes)
endif(KICAD_BUILD_STATIC)

# <SOURCE_DIR> = ${PREFIX}/src/glew
# There is a Bazaar 'boost scratch repo' in <SOURCE_DIR>/boost and after committing pristine
# download, the patch is applied.  This lets you regenerate a new patch at any time
# easily, simply by editing the working tree in <SOURCE_DIR> and doing "bzr diff" in there.

ExternalProject_Add( pixman
    PREFIX          "${PREFIX}"
    DOWNLOAD_DIR    "${DOWNLOAD_DIR}"
    URL             http://cairographics.org/releases/pixman-${PIXMAN_RELEASE}.tar.gz
    URL_MD5         ${PIXMAN_MD5}
    STAMP_DIR       "${PREFIX}"

    #SOURCE_DIR      "${PREFIX}"
    BUILD_IN_SOURCE 1

    UPDATE_COMMAND  ${CMAKE_COMMAND} -E remove_directory "${PIXMAN_ROOT}"

    #PATCH_COMMAND     "true"
    CONFIGURE_COMMAND ./configure --prefix=${PIXMAN_ROOT} ${PIXMAN_OPTS} ${PIXMAN_CPPFLAGS}  --disable-dependency-tracking
    #BINARY_DIR      "${PREFIX}"

    BUILD_COMMAND   $(MAKE)

    INSTALL_DIR     "${PIXMAN_ROOT}"
    INSTALL_COMMAND $(MAKE) install
    )
