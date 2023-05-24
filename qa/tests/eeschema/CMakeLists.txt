#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2017 CERN
# @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# or you may search the http://www.gnu.org website for the version 2 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA


include_directories( BEFORE ${INC_BEFORE} )

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/qa/mocks/include
    ${CMAKE_SOURCE_DIR}/qa/qa_utils
    ${CMAKE_SOURCE_DIR}/qa
    ${INC_AFTER}
    )

set( QA_EESCHEMA_SRCS
    # need the mock Pgm for many functions
    ${CMAKE_SOURCE_DIR}/qa/mocks/kicad/common_mocks.cpp

    # The main test entry points
    test_module.cpp

    # Base internal units (1=100nm) testing.
    test_sch_biu.cpp

    # Shared between programs, but dependent on the BIU
    ${CMAKE_SOURCE_DIR}/qa/tests/common/test_format_units.cpp
    ${CMAKE_SOURCE_DIR}/qa/tests/common/test_array_options.cpp

    sch_plugins/altium/test_altium_parser_sch.cpp

	erc/test_erc_label_not_connected.cpp
	erc/test_erc_stacking_pins.cpp
	erc/test_erc_global_labels.cpp
	erc/test_erc_no_connect.cpp
    erc/test_erc_hierarchical_schematics.cpp

    test_eagle_plugin.cpp
    test_lib_part.cpp
    test_netlist_exporter_kicad.cpp
    test_ee_item.cpp
    test_legacy_power_symbols.cpp
    test_pin_numbers.cpp
    test_sch_netclass.cpp
    test_sch_pin.cpp
    test_sch_rtree.cpp
    test_sch_reference_list.cpp
    test_sch_sheet.cpp
    test_sch_sheet_path.cpp
    test_sch_sheet_list.cpp
    test_sch_symbol.cpp
)

if( WIN32 )
    # We want to declare a resource manifest on Windows to enable UTF8 mode
    # Without UTF8 mode, some random IO tests may fail, we set the active code page on normal kicad to UTF8 as well
    if( MINGW )
        # QA_EESCHEMA_RESOURCES variable is set by the macro.
        mingw_resource_compiler( qa_eeschema )
    else()
        set( QA_EESCHEMA_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/qa_eeschema.rc )
    endif()
endif()

add_executable( qa_eeschema
    ${QA_EESCHEMA_SRCS}
    ${QA_EESCHEMA_RESOURCES}
)

target_link_libraries( qa_eeschema
PRIVATE
    eeschema_kiface_objects
    common
    pcbcommon
    3d-viewer
    scripting
    kimath
    qa_utils
    qa_schematic_utils
    markdown_lib
    ${GDI_PLUS_LIBRARIES}
    Boost::headers
    Boost::unit_test_framework
)

# Eeschema tests, so pretend to be eeschema (for units, etc)
target_compile_definitions( qa_eeschema
    PUBLIC EESCHEMA
)

kicad_add_boost_test( qa_eeschema qa_eeschema )

