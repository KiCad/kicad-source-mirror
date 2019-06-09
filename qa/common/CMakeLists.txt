# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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

find_package( wxWidgets 3.0.0 COMPONENTS gl aui adv html core net base xml stc REQUIRED )

set( common_srcs
    # This is needed for the global mock objects
    common_mocks.cpp

    # The main test entry points
    test_module.cpp

    # stuff from common due to...units?
    ../../common/eda_text.cpp
    ../../common/base_units.cpp

    # stuff from common which is needed...why?
    ../../common/colors.cpp
    ../../common/observable.cpp

    wximage_test_utils.cpp

    test_array_axis.cpp
    test_array_options.cpp
    test_bitmap_base.cpp
    test_color4d.cpp
    test_coroutine.cpp
    test_format_units.cpp
    test_lib_table.cpp
    test_kicad_string.cpp
    test_refdes_utils.cpp
    test_title_block.cpp
    test_utf8.cpp
    test_wildcards_and_files_ext.cpp
    test_wx_filename.cpp

    libeval/test_numeric_evaluator.cpp

    geometry/test_fillet.cpp
    geometry/test_segment.cpp
    geometry/test_shape_arc.cpp
    geometry/test_shape_poly_set_collision.cpp
    geometry/test_shape_poly_set_distance.cpp
    geometry/test_shape_poly_set_iterator.cpp

    view/test_zoom_controller.cpp
)

set( common_libs
    common
    gal
    qa_utils
    unit_test_utils
    ${wxWidgets_LIBRARIES}
)


# Use code with GERBVIEW defines in place (primarily IU difference)
add_executable( qa_common_gerbview ${common_srcs} )
target_link_libraries( qa_common_gerbview ${common_libs} )
target_compile_definitions( qa_common_gerbview PRIVATE GERBVIEW )

# Use code with PCBNEW defines in place
add_executable( qa_common_pcbnew ${common_srcs} )
target_link_libraries( qa_common_pcbnew  ${common_libs} )
target_compile_definitions( qa_common_pcbnew PRIVATE PCBNEW )

# Use code with EESCHEMA defines in place
add_executable( qa_common_eeschema ${common_srcs} )
target_link_libraries( qa_common_eeschema  ${common_libs} )
target_compile_definitions( qa_common_eeschema PRIVATE EESCHEMA )

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/include
    ${INC_AFTER}
)

kicad_add_boost_test( qa_common_eeschema common_eeschema )
kicad_add_boost_test( qa_common_pcbnew common_pcbnew )
kicad_add_boost_test( qa_common_gerbview qa_common_gerbview )