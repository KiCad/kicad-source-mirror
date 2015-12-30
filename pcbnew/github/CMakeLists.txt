#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
#  Copyright (C) 2013-2015 Kicad Developers, see AUTHORS.txt for contributors.
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

# These are additions to any inherited from pcbnew dir:
include_directories( . )

# Tell AVHTTP we have SSL.
set( GITHUB_PLUGIN_SRCS
    github_plugin.cpp 
    github_getliblist.cpp
    html_link_parser.cpp
    )

add_library( github_plugin STATIC ${GITHUB_PLUGIN_SRCS} )

target_link_libraries( github_plugin
    common
    )

if( MINGW )
    target_link_libraries( github_plugin
        ${wxWidgets_LIBRARIES}
        ws2_32
        )
endif()

add_dependencies( github_plugin boost )

