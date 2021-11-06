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

    wximage_test_utils.cpp

    test_array_axis.cpp
    test_bitmap_base.cpp
    test_color4d.cpp
    test_coroutine.cpp
    test_eda_rect.cpp
    test_lib_table.cpp
    test_kicad_string.cpp
    test_property.cpp
    test_refdes_utils.cpp
    test_title_block.cpp
    test_utf8.cpp
    test_wildcards_and_files_ext.cpp
    test_wx_filename.cpp

    libeval/test_numeric_evaluator.cpp

    plugins/altium/test_altium_parser.cpp
    plugins/altium/test_altium_parser_utils.cpp

    view/test_zoom_controller.cpp
)

set( common_libs
    common
    gal
    qa_utils
    ${wxWidgets_LIBRARIES}
)


# Test executable for non-program-specific code
add_executable( qa_common ${common_srcs} )
target_link_libraries( qa_common ${common_libs} )
target_include_directories( qa_common PRIVATE
    $<TARGET_PROPERTY:libcontext,INTERFACE_INCLUDE_DIRECTORIES>
)


include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/include
    ${INC_AFTER}
)

kicad_add_boost_test( qa_common qa_common )
