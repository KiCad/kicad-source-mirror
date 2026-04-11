#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2022 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
# Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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


add_executable( test_pegtl
    main.cpp
  )

add_dependencies( test_pegtl pegtl )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/thirdparty/pegtl/
    ${INC_AFTER}
)

