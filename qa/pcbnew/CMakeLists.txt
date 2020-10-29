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

set( QA_PCBNEW_SRCS
    # The main test entry points
    test_module.cpp

    # Shared between programs, but dependent on the BIU
    ${CMAKE_SOURCE_DIR}/qa/common/test_format_units.cpp
    ${CMAKE_SOURCE_DIR}/qa/common/test_array_options.cpp

    # testing utility routines
    board_test_utils.cpp
    drc/drc_test_utils.cpp

    # test compilation units (start test_)
    test_array_pad_name_provider.cpp
    test_graphics_import_mgr.cpp
    test_lset.cpp
    test_pad_naming.cpp
    test_libeval_compiler.cpp

    drc/test_drc_courtyard_invalid.cpp
    drc/test_drc_courtyard_overlap.cpp

    group_saveload.cpp
)

add_executable( qa_pcbnew
    ${QA_PCBNEW_SRCS}

    # Older CMakes cannot link OBJECT libraries
    # https://cmake.org/pipermail/cmake/2013-November/056263.html
    $<TARGET_OBJECTS:pcbnew_kiface_objects>
)

# Pcbnew tests, so pretend to be pcbnew (for units, etc)
target_compile_definitions( qa_pcbnew
    PRIVATE PCBNEW
)

# Anytime we link to the kiface_objects, we have to add a dependency on the last object
# to ensure that the generated lexer files are finished being used before the qa runs in a
# multi-threaded build
add_dependencies( qa_pcbnew pcbnew )

target_link_libraries( qa_pcbnew
    qa_pcbnew_utils
    3d-viewer
    connectivity
    pcbcommon
    pnsrouter
    gal
    common
    gal
    qa_utils
    dxflib_qcad
    tinyspline_lib
    nanosvg
    idf3
    unit_test_utils
    markdown_lib
    ${PCBNEW_IO_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
)

kicad_add_boost_test( qa_pcbnew qa_pcbnew )
