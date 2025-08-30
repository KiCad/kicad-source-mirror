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

set( QA_KIMATH_SRCS
    kimath_test_module.cpp

    test_kimath.cpp

    geometry/geom_test_utils.cpp
    geometry/test_chamfer.cpp
    geometry/test_circle.cpp
    geometry/test_distribute.cpp
    geometry/test_dogbone.cpp
    geometry/test_eda_angle.cpp
    geometry/test_ellipse_to_bezier.cpp
    geometry/test_fillet.cpp
    geometry/test_half_line.cpp
    geometry/test_oval.cpp
    geometry/test_poly_triangulation.cpp
    geometry/test_segment.cpp
    geometry/test_shape_compound_collision.cpp
    geometry/test_shape_arc.cpp
    geometry/test_shape_nearest_points.cpp
    geometry/test_shape_poly_set.cpp
    geometry/test_shape_poly_set_arcs.cpp
    geometry/test_shape_poly_set_collision.cpp
    geometry/test_shape_poly_set_distance.cpp
    geometry/test_shape_poly_set_iterator.cpp
    geometry/test_shape_poly_set_split_outlines.cpp
    geometry/test_shape_line_chain.cpp
    geometry/test_shape_line_chain_collision.cpp
    geometry/test_vector_utils.cpp
    geometry/test_shape_rect_corner.cpp

    math/test_box2.cpp
    math/test_matrix3x3.cpp
    math/test_vector2.cpp
    math/test_vector3.cpp
    math/test_util.cpp
)

if( WIN32 )
    # We want to declare a resource manifest on Windows to enable UTF8 mode
    # Without UTF8 mode, some random IO tests may fail, we set the active code page on normal kicad to UTF8 as well
    if( MINGW )
        # QA_KIMATH_RESOURCES variable is set by the macro.
        mingw_resource_compiler( qa_kimath )
    else()
        set( QA_KIMATH_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/qa_kimath.rc )
    endif()
endif()

add_executable( qa_kimath
    ${QA_KIMATH_SRCS}
    ${QA_KIMATH_RESOURCES}

    # Mock Pgm needed for advanced_config
    ${CMAKE_SOURCE_DIR}/qa/mocks/kicad/common_mocks.cpp
)

target_link_libraries( qa_kimath
    qa_utils
    kimath
    Boost::headers
    Boost::unit_test_framework
    ${wxWidgets_LIBRARIES}
)

target_include_directories( qa_kimath PRIVATE
    ${CMAKE_SOURCE_DIR}/qa/mocks/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

kicad_add_boost_test( qa_kimath qa_kimath )

setup_qa_env( qa_kimath )