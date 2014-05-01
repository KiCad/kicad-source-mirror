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



# Download av_http and install into ${PREFIX}, typically in our KiCad source tree.
# Assumes include( ExternalProject ) was done inline previous to this file
# and that set( DOWNLOAD_DIR ... ) was set in a higher context.

#-----<configure>-------------------------------------------------------------------------------------

# soon cmake will have https support, switch to a true download then:
#set( AVHTTP_RELEASE ??? )
#set( AVHTTP_MD5 ???? ) # re-calc this on every RELEASE change

#-----</configure>-----------------------------------------------------------------------------------


# Where the library is to be installed.
set( PREFIX ${DOWNLOAD_DIR}/avhttp )

if( KICAD_SKIP_BOOST )
    set( AVHTTP_DEPEND "" )
else()
    set( AVHTTP_DEPEND "boost" )
endif()


# Install the AVHTTP header only library ${PREFIX}
ExternalProject_Add( avhttp
    PREFIX          ${PREFIX}
    DOWNLOAD_DIR    ${DOWNLOAD_DIR}     # no true download yet

    # grab it from a local zip file for now, cmake caller's source dir
    URL             ${CMAKE_CURRENT_SOURCE_DIR}/avhttp-master.zip
    DEPENDS         ${AVHTTP_DEPEND}

    CONFIGURE_COMMAND ""

    BUILD_COMMAND   ""
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR> <INSTALL_DIR>
    )


set( AVHTTP_INCLUDE_DIR  "${PREFIX}/include" CACHE FILEPATH "AVHTTP include directory" )
mark_as_advanced( AVHTTP_INCLUDE_DIR )
