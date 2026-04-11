# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2014-2019 KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.


# add_subdirectory( libeval_compiler )

if( KICAD_DRC_PROTO )
    add_subdirectory( drc_proto )
endif()

# Utility/debugging/profiling programs
add_subdirectory( common_tools )
add_subdirectory( pcbnew_tools )

if( KICAD_BUILD_PEGTL_DEBUG_TOOL )
    add_subdirectory( pegtl )
endif()

if( KICAD_BUILD_PNS_DEBUG_TOOL )
    add_subdirectory( pns )
endif()

