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

# Downloads and builds PCRE

#-----<configure>----------------------------------------------------------------

set( PCRE_RELEASE 8.34 )
set( PCRE_MD5 eb34b2c9c727fd64940d6fd9a00995eb )   # re-calc this on every RELEASE change

set( PCRE_ROOT "${PROJECT_SOURCE_DIR}/pcre_root" )

#-----</configure>---------------------------------------------------------------

find_package( BZip2 REQUIRED )

set( PREFIX ${DOWNLOAD_DIR}/pcre )

if (APPLE) 
    if( CMAKE_OSX_ARCHITECTURES )
        set( PCRE_CFLAGS  "CFLAGS=-arch ${CMAKE_OSX_ARCHITECTURES} -mmacosx-version-min=10.5" )
        set( PCRE_CXXFLAGS "CXXFLAGS=-arch ${CMAKE_OSX_ARCHITECTURES} -mmacosx-version-min=10.5" )
        set( PCRE_LDFLAGS "LDFLAGS=-arch ${CMAKE_OSX_ARCHITECTURES} -mmacosx-version-min=10.5" )
    endif( CMAKE_OSX_ARCHITECTURES )
endif(APPLE)

ExternalProject_Add( pcre
    PREFIX          "${PREFIX}"
    DOWNLOAD_DIR    "${DOWNLOAD_DIR}"
    URL             http://sourceforge.net/projects/pcre/files/pcre/${PCRE_RELEASE}/pcre-${PCRE_RELEASE}.tar.gz
    URL_MD5         ${PCRE_MD5}
    STAMP_DIR       "${PREFIX}"

    #SOURCE_DIR      "${PREFIX}"
    BUILD_IN_SOURCE 1

    UPDATE_COMMAND  ${CMAKE_COMMAND} -E remove_directory "${PCRE_ROOT}"

    #PATCH_COMMAND     "true"
    CONFIGURE_COMMAND ./configure --prefix=${PCRE_ROOT} ${PCRE_CFLAGS} ${PCRE_CXXFLAGS} ${PCRE_LDFLAGS} --disable-dependency-tracking

    #BINARY_DIR      "${PREFIX}"

    BUILD_COMMAND   $(MAKE) 

    INSTALL_DIR      "${PCRE_ROOT}"
    INSTALL_COMMAND  $(MAKE) install
    )
