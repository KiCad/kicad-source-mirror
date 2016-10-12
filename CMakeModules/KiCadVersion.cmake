#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2016 Wayne Stambaugh <stambaughw@verizon.net>
#  Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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
# the KiCad version when the source is provided in an archive file.
# When KiCad is cloned using git, the git version is used.  The only
# time this should be set to a value other than "no-vcs-found" is when
# a source archive is created.  This eliminates the need to set
# KICAD_BUILD_VERSION during the build configuration step.
set( _wvh_version_str "no-vcs-found" )

# Set the KiCad branch name to stable for stable source releases.
set( KICAD_BRANCH_NAME "undefined" )
