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



# Download and patch boost headers to a particular version.
# Assumes include( ExternalProject ) was done inline previous to this file.

set( BOOST_RELEASE 1.53.0 )
set( BOOST_MD5 a00d22605d5dbcfb4c9936a9b35bc4c2 ) # re-calc this on every RELEASE change

string( REGEX REPLACE "\\." "_" BOOST_VERS "${BOOST_RELEASE}" )
set( PREFIX ${DOWNLOAD_DIR}/boost_${BOOST_VERS} )


ExternalProject_Add( boost
    PREFIX          ${PREFIX}
    DOWNLOAD_DIR    ${DOWNLOAD_DIR}
    URL             http://downloads.sourceforge.net/project/boost/boost/${BOOST_RELEASE}/boost_${BOOST_VERS}.tar.bz2
    URL_MD5         ${BOOST_MD5}

    # The patch command executes with the working directory set to <SOURCE_DIR>
    PATCH_COMMAND   bzr patch -p0 ${PROJECT_SOURCE_DIR}/patches/boost.patch

    CONFIGURE_COMMAND ""

    # remove then re-copy into the include/boost directory during next two steps:
    BUILD_COMMAND   ${CMAKE_COMMAND} -E remove_directory ${PROJECT_SOURCE_DIR}/include/boost
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/boost ${PROJECT_SOURCE_DIR}/include/boost
    )


# <SOURCE_DIR> = ${PREFIX}/src/boost
# Add extra steps, so that we can easily regenerate any boost patch needed for the above.
# There is a Bazaar 'boost scratch repo' in <SOURCE_DIR> and after committing pristine
# download, the patch is applied.  This lets you regenerate a new patch at any time
# easily, simply by editing the working tree in <SOURCE_DIR> and doing "bzr diff" in there.


ExternalProject_Add_Step( boost bzr_commit_boost
    COMMAND bzr ci -q -m pristine <SOURCE_DIR>
    COMMENT "committing pristine boost files to 'boost scratch repo'"
    DEPENDERS patch
    )


ExternalProject_Add_Step( boost bzr_add_boost
    COMMAND bzr add -q <SOURCE_DIR>
    COMMENT "adding pristine boost files to 'boost scratch repo'"
    DEPENDERS bzr_commit_boost
    )


ExternalProject_Add_Step( boost bzr_init_boost
    COMMAND bzr init -q <SOURCE_DIR>
    COMMENT "creating 'boost scratch repo' specifically for boost to track boost patches"
    DEPENDERS bzr_add_boost
    DEPENDEES download
    )
