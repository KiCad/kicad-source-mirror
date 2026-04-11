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
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

# This file will configure the linux metainfo.xml file to include the version
# and build date.

# It requires the following variables to be defined before its call:
# SRC_PATH - The root directory for the source
# BUILD_PATH - The root directory for the build directory

message( STATUS "Generating linux metainfo file" )

# Create the KiCad version strings
include( ${KICAD_CMAKE_MODULE_PATH}/KiCadVersion.cmake )
include( ${KICAD_CMAKE_MODULE_PATH}/KiCadFullVersion.cmake )

# Create the date of the configure
if( KICAD_COMMIT_DATE STREQUAL "0")
    message(STATUS "  No git commit date found, using timestamp")
    string( TIMESTAMP KICAD_CONFIG_TIMESTAMP "%Y-%m-%d" )
else()
    set( KICAD_CONFIG_TIMESTAMP "${KICAD_COMMIT_DATE}" )
endif()

message( STATUS "  date: ${KICAD_CONFIG_TIMESTAMP}" )
message( STATUS "  version: ${KICAD_VERSION_FULL}" )

# Configure the KiCad metainfo file
configure_file( ${SRC_PATH}/resources/linux/metainfo/org.kicad.kicad.metainfo.xml.in
                ${BUILD_PATH}/resources/linux/metainfo/org.kicad.kicad.metainfo.xml.in
                @ONLY )

# Ensure the file was configured successfully
if( NOT EXISTS ${BUILD_PATH}/resources/linux/metainfo/org.kicad.kicad.metainfo.xml.in )
    message( FATAL_ERROR "Configuration failed to write file org.kicad.kicad.metainfo.xml.in" )
endif()
