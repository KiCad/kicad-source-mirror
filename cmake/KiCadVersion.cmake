#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2016 Wayne Stambaugh <stambaughw@gmail.com>
#  Copyright (C) 2016-2023, 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

# Note: unless you are the person responsible for creating releases,
#       do *not* change these variables.  This way the KiCad project
#       can maintain control over what is an official KiCad build and
#       what is not.  Setting these variable that conflict with KiCad
#       releases is a shooting offense.
#
# This file gets included in the WriteVersionHeader.cmake file to set
# the default KiCad version when the source is provided in an archive
# file or git is not available on the build system.   When KiCad is
# cloned using git, the git version is used.  This version string should
# be set after each version tag is added to the git repo.  This will
# give developers a reasonable idea where which branch was used to build
# KiCad.
#
# Note: This version string should follow the semantic versioning system
set( KICAD_SEMANTIC_VERSION "9.99.0-unknown" )

# Default the version to the semantic version.
# This is overridden by the git repository tag though (if using git)
set( KICAD_VERSION "${KICAD_SEMANTIC_VERSION}" )
