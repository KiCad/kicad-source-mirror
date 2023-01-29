# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2023 KiCad Developers, see AUTHORS.TXT for contributors.
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

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ${INC_AFTER}
    )

add_executable( enum_exporter WIN32
    enum_exporter.cpp
    )

target_link_libraries( enum_exporter
    common
    )

target_include_directories( enum_exporter PRIVATE
    $<TARGET_PROPERTY:magic_enum,INTERFACE_INCLUDE_DIRECTORIES>
    )

if( MSVC )
   # The cli needs subsystem:console or else we can't link wmain/main
   set_target_properties(enum_exporter PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
   set_target_properties(enum_exporter PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
   set_target_properties(enum_exporter PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
   set_target_properties(enum_exporter PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
   set_target_properties(enum_exporter PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
   set_target_properties(enum_exporter PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")
endif()
