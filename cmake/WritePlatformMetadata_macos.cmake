#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2019 Ian McInerney <Ian.S.McInerney@ieee.org>
#  Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

# This file will configure the MacOS info.plist files to include the version
# and build date.

message( STATUS "Creating MacOS metadata" )

# Create the KiCad version strings
set( SRC_PATH ${PROJECT_SOURCE_DIR} )
include( ${KICAD_CMAKE_MODULE_PATH}/KiCadVersion.cmake )
include( ${KICAD_CMAKE_MODULE_PATH}/KiCadFullVersion.cmake )


# Configure each plist file from the respurces directory and store it in the build directory
configure_file( ${PROJECT_SOURCE_DIR}/resources/macos/plist/bitmap2component.Info.plist.in
                ${PROJECT_BINARY_DIR}/bitmap2component/Info.plist
                @ONLY )

configure_file( ${PROJECT_SOURCE_DIR}/resources/macos/plist/eeschema.Info.plist.in
                ${PROJECT_BINARY_DIR}/eeschema/Info.plist
                @ONLY )

configure_file( ${PROJECT_SOURCE_DIR}/resources/macos/plist/gerbview.Info.plist.in
                ${PROJECT_BINARY_DIR}/gerbview/Info.plist
                @ONLY )

configure_file( ${PROJECT_SOURCE_DIR}/resources/macos/plist/kicad.Info.plist.in
                ${PROJECT_BINARY_DIR}/kicad/Info.plist
                @ONLY )

configure_file( ${PROJECT_SOURCE_DIR}/resources/macos/plist/pcb_calculator.Info.plist.in
                ${PROJECT_BINARY_DIR}/pcb_calculator/Info.plist
                @ONLY )

configure_file( ${PROJECT_SOURCE_DIR}/resources/macos/plist/pcbnew.Info.plist.in
                ${PROJECT_BINARY_DIR}/pcbnew/Info.plist
                @ONLY )

configure_file( ${PROJECT_SOURCE_DIR}/resources/macos/plist/pleditor.Info.plist.in
                ${PROJECT_BINARY_DIR}/pagelayout_editor/Info.plist
                @ONLY )
