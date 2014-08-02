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

# Downloads and builds GLEW

#-----<configure>----------------------------------------------------------------

set( GLEW_RELEASE 1.10.0 )
set( GLEW_MD5 2f09e5e6cb1b9f3611bcac79bc9c2d5d )   # re-calc this on every RELEASE change

# The boost headers [and static libs if built] go here, at the top of KiCad
# source tree in boost_root.
set( GLEW_ROOT "${PROJECT_SOURCE_DIR}/glew_root" )

#-----</configure>---------------------------------------------------------------

if( NOT BZIP2_FOUND )
    find_package( BZip2 REQUIRED )
endif()

set( PREFIX ${DOWNLOAD_DIR}/glew )

if (APPLE)
    if( CMAKE_OSX_ARCHITECTURES )
        set( GLEW_CFLAGS  "CFLAGS.EXTRA=-arch ${CMAKE_OSX_ARCHITECTURES} -mmacosx-version-min=10.5" )
        set( GLEW_LDFLAGS "LDFLAGS.EXTRA=-arch ${CMAKE_OSX_ARCHITECTURES} -mmacosx-version-min=10.5" )
        set( GLEW_STRIP   "STRIP=")
    endif( CMAKE_OSX_ARCHITECTURES )
endif(APPLE)

# <SOURCE_DIR> = ${PREFIX}/src/glew
# There is a Bazaar 'boost scratch repo' in <SOURCE_DIR>/boost and after committing pristine
# download, the patch is applied.  This lets you regenerate a new patch at any time
# easily, simply by editing the working tree in <SOURCE_DIR> and doing "bzr diff" in there.

ExternalProject_Add( glew
    PREFIX          "${PREFIX}"
    DOWNLOAD_DIR    "${DOWNLOAD_DIR}"
    URL             http://sourceforge.net/projects/glew/files/glew/1.10.0/glew-${GLEW_RELEASE}.tgz
    URL_MD5         ${GLEW_MD5}
    STAMP_DIR       "${PREFIX}"

    #SOURCE_DIR      "${PREFIX}"
    BUILD_IN_SOURCE 1

    UPDATE_COMMAND  ${CMAKE_COMMAND} -E remove_directory "${GLEW_ROOT}"

    #PATCH_COMMAND     "true"
    CONFIGURE_COMMAND ""

    #BINARY_DIR      "${PREFIX}"

    BUILD_COMMAND   $(MAKE) ${GLEW_CFLAGS} ${GLEW_LDFLAGS} ${GLEW_STRIP}

    INSTALL_DIR      "${GLEW_ROOT}"
    INSTALL_COMMAND  $(MAKE) GLEW_DEST="${GLEW_ROOT}" install
    )

#
# Optional Steps
#

if( APPLE )
# On OSX is needed to run ranlib to make .a indexes for all platforms
ExternalProject_Add_Step( glew glew_osx_ranlib
    COMMAND ranlib "${GLEW_ROOT}/lib/libGLEW.a"
    COMMENT "ranlib ${GLEW_ROOT}/lib/libGLEW.a - Needed on OSX only"
    DEPENDEES install
    )
endif()
