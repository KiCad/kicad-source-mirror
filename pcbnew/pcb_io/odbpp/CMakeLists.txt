# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2024 KiCad Developers, see AUTHORS.TXT for contributors.
# Author: SYSUEric <jzzhuang666@gmail.com>.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.

# Sources for the pcbnew pcb_io called ODB++

include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )


set( ODBPP_SRCS
    odb_util.cpp
    odb_attribute.cpp
    odb_feature.cpp
    odb_component.cpp
    odb_netlist.cpp
    odb_eda_data.cpp
    odb_fonts.cpp
    odb_entity.cpp
    pcb_io_odbpp.cpp
    )

add_library( odbpp STATIC ${ODBPP_SRCS} )

target_link_libraries( odbpp pcbcommon )
