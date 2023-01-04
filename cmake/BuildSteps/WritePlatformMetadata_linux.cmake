#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2019-2020 Ian McInerney <Ian.S.McInerney@ieee.org>
#  Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#

# This file will configure the linux metainfo.xml file to include the version
# and build date.

# It requires the following variables to be defined before its call:
# SRC_PATH - The root directory for the source
# BUILD_PATH - The root directory for the build directory

message( STATUS "Creating linux metadata" )

# Create the KiCad version strings
include( ${KICAD_CMAKE_MODULE_PATH}/KiCadVersion.cmake )
include( ${KICAD_CMAKE_MODULE_PATH}/KiCadFullVersion.cmake )

# Create the date of the configure
string( TIMESTAMP KICAD_CONFIG_TIMESTAMP "%Y-%m-%d" )

# Configure the KiCad metainfo file
configure_file( ${SRC_PATH}/resources/linux/metainfo/org.kicad.kicad.metainfo.xml.in
                ${BUILD_PATH}/resources/linux/metainfo/org.kicad.kicad.metainfo.xml.in
                @ONLY )

# Ensure the file was configured successfully
if( NOT EXISTS ${BUILD_PATH}/resources/linux/metainfo/org.kicad.kicad.metainfo.xml.in )
    message( FATAL_ERROR "Configuration failed to write file org.kicad.kicad.metainfo.xml.in" )
endif()
