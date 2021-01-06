# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2020 KiCad Developers, see AUTHORS.TXT for contributors.
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
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# or you may search the http://www.gnu.org website for the version 2 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

#
# Unit tests for KiCad math routines.

set( KIMATH_SRCS
    kimath_test_module.cpp

    test_kimath.cpp

    geometry/test_fillet.cpp
    geometry/test_circle.cpp
    geometry/test_segment.cpp
    geometry/test_shape_compound_collision.cpp
    geometry/test_shape_arc.cpp
    geometry/test_shape_poly_set_collision.cpp
    geometry/test_shape_poly_set_distance.cpp
    geometry/test_shape_poly_set_iterator.cpp
    geometry/test_poly_grid_partition.cpp
    geometry/test_shape_line_chain.cpp

    math/test_vector2.cpp
    math/test_vector3.cpp
)

add_executable( qa_kimath ${KIMATH_SRCS} )

target_link_libraries( qa_kimath
    qa_utils
    kimath
    unit_test_utils
    ${wxWidgets_LIBRARIES}
)

target_include_directories( qa_kimath PRIVATE
    ${CMAKE_SOURCE_DIR}/include         # Needed for core/optional.h
    ${CMAKE_CURRENT_SOURCE_DIR}
)

kicad_add_boost_test( qa_kimath qa_kimath )
