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

set( CAIRO_RELEASE 1.12.0 )
set( CAIRO_MD5 e6c85575ba7094f88b637bdfd835a751 )   # re-calc this on every RELEASE change

# The boost headers [and static libs if built] go here, at the top of KiCad
# source tree in boost_root.
set( CAIRO_ROOT "${PROJECT_SOURCE_DIR}/cairo_root" )

#-----</configure>---------------------------------------------------------------

if( NOT BZIP2_FOUND )
    find_package( BZip2 REQUIRED )
endif()

set( PREFIX ${DOWNLOAD_DIR}/cairo )

if ( KICAD_BUILD_STATIC )
    set( CAIRO_BUILDTYPE --disable-shared )
endif( KICAD_BUILD_STATIC )


if (APPLE)

    set( CAIRO_CFLAGS  "CFLAGS=" )
    set( CAIRO_LDFLAGS "LDFLAGS=-framework CoreServices -framework Cocoa" )
    set( CAIRO_OPTS    --enable-ft=no )

    if( CMAKE_OSX_ARCHITECTURES )
        set( CAIRO_CFLAGS  "${CAIRO_CFLAGS} -arch ${CMAKE_OSX_ARCHITECTURES}" )
        set( CAIRO_LDFLAGS "${CAIRO_LDFLAGS} -arch ${CMAKE_OSX_ARCHITECTURES}" )
    endif( CMAKE_OSX_ARCHITECTURES )

    if( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
        set(CAIRO_CFLAGS  "${CAIRO_CFLAGS} -fno-lto" )
    endif()

    if( CMAKE_OSX_SYSROOT )
        #set( CAIRO_CFLAGS  "${CAIRO_CFLAGS}  -isysroot ${CMAKE_OSX_SYSROOT}")
        set( CAIRO_LDFLAGS "${CAIRO_LDFLAGS}  -isysroot ${CMAKE_OSX_SYSROOT}")
    endif( CMAKE_OSX_SYSROOT)

endif(APPLE)

# <SOURCE_DIR> = ${PREFIX}/src/glew
# There is a Bazaar 'boost scratch repo' in <SOURCE_DIR>/boost and after committing pristine
# download, the patch is applied.  This lets you regenerate a new patch at any time
# easily, simply by editing the working tree in <SOURCE_DIR> and doing "bzr diff" in there.

ExternalProject_Add(  cairo
    PREFIX            "${PREFIX}"
    DOWNLOAD_DIR      "${DOWNLOAD_DIR}"
    URL               http://cairographics.org/releases/cairo-${CAIRO_RELEASE}.tar.gz
    URL_MD5           ${CAIRO_MD5}
    STAMP_DIR         "${PREFIX}"

    DEPENDS           pkgconfig pixman libpng

    BUILD_IN_SOURCE   1
    #SOURCE_DIR       "${PREFIX}"
    #PATCH_COMMAND    ""
    UPDATE_COMMAND  ${CMAKE_COMMAND} -E remove_directory "${CAIRO_ROOT}"

    CONFIGURE_COMMAND ./configure --prefix=${CAIRO_ROOT} --enable-static ${CAIRO_BUILDTYPE}
                      PKG_CONFIG=${PROJECT_SOURCE_DIR}/pkgconfig_root/bin/pkg-config
                      PKG_CONFIG_PATH=${PROJECT_SOURCE_DIR}/pixman_root/lib/pkgconfig:${PROJECT_SOURCE_DIR}/libpng_root/lib/pkgconfig
                      --enable-png=yes --enable-svg=yes
                      --disable-silent-rules --disable-dependency-tracking
                      ${CAIRO_CFLAGS}
                      ${CAIRO_LDFLAGS}
                      ${CAIRO_OPTS}

    #BINARY_DIR      "${PREFIX}"

    BUILD_COMMAND   $(MAKE)

    INSTALL_DIR     "${CAIRO_ROOT}"
    INSTALL_COMMAND $(MAKE) install
    )

# match these with whatever FindCairo.cmake sets
# Dick i'vent set it because /lib and /lib64 issue in non multiarch binaries OSs
#set( CAIRO_FOUND true )
set( CAIRO_INCLUDE_DIR ${CAIRO_ROOT}/include )
set( CAIRO_LIBRARIES ${CAIRO_ROOT}/lib )
