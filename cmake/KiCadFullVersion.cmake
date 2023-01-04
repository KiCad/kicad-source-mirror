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

# This file will create the full KiCad version string. The base of this string
# will be either the git tag followed by the commit hash (if built inside a git
# repository), or the version from KiCadVersion.cmake. The user-provided
# KICAD_VERSION_EXTRA is then appended to the base version string.

# Use git to determine the version string if it's available.
include( ${KICAD_CMAKE_MODULE_PATH}/CreateGitVersionHeader.cmake )
create_git_version_header( ${SRC_PATH} )

# $KICAD_VERSION is set in KiCadVersion.cmake or by git (if it is available).
set( KICAD_VERSION_FULL "${KICAD_VERSION}" )

# Optional user version information defined at configuration.
if( KICAD_VERSION_EXTRA )
    set( KICAD_VERSION_FULL "${KICAD_VERSION_FULL}-${KICAD_VERSION_EXTRA}" )
endif()
