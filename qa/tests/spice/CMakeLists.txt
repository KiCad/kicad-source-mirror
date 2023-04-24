#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2023 Ian McInerney
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

set( INC_AFTER ${INC_AFTER} ${NGSPICE_INCLUDE_DIR} )

# Find out the exact libngspice file name
get_filename_component( NGSPICE_DLL_ABSPATH "${NGSPICE_DLL}" ABSOLUTE )
get_filename_component( NGSPICE_DLL_FILE "${NGSPICE_DLL_ABSPATH}" NAME )

set_property( SOURCE sim/ngspice.cpp
    APPEND PROPERTY COMPILE_DEFINITIONS
    NGSPICE_DLL_FILE="${NGSPICE_DLL_FILE}"
    )

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/qa/mocks/include
    ${CMAKE_SOURCE_DIR}/qa/qa_utils
    ${CMAKE_SOURCE_DIR}/qa
    ${INC_AFTER}
    )

set( QA_SPICE_SRCS
    # need the mock Pgm for many functions
    ${CMAKE_SOURCE_DIR}/qa/mocks/kicad/common_mocks.cpp

    # The main test entry points
    test_module.cpp

    # Test exporting the netlist
    test_netlist_exporter_spice.cpp

    # Test SPICE library and models
    test_library_spice.cpp
    test_sim_model_inference.cpp
    test_sim_model_ngspice.cpp
    test_sim_regressions.cpp

    test_ngspice_helpers.cpp
)

if( WIN32 )
    # We want to declare a resource manifest on Windows to enable UTF8 mode
    # Without UTF8 mode, some random IO tests may fail, we set the active code page on normal kicad to UTF8 as well
    if( MINGW )
        # QA_SPICE_RESOURCES variable is set by the macro.
        mingw_resource_compiler( qa_spice )
    else()
        set( QA_SPICE_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/qa_spice.rc )
    endif()
endif()

add_executable( qa_spice
    ${QA_SPICE_SRCS}
    ${QA_SPICE_RESOURCES}
)

target_link_libraries( qa_spice
PRIVATE
    eeschema_kiface_objects
    common
    pcbcommon
    scripting
    kimath
    qa_utils
    qa_schematic_utils
    markdown_lib
    ${NGSPICE_LIBRARY}
    ${GDI_PLUS_LIBRARIES}
    Boost::headers
    Boost::unit_test_framework
)

if( KICAD_SPICE_QA AND MSVC )
    # Allow for MSVC to run from the build directory
    add_custom_command( TARGET qa_spice POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${NGSPICE_DLL}" "$<TARGET_FILE_DIR:qa_spice>"
        )

    add_custom_command( TARGET qa_spice POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${NGSPICE_CM_DIR}" "$<TARGET_FILE_DIR:qa_spice>/ngspice"
        )
endif()


# Eeschema tests, so pretend to be eeschema (for units, etc)
target_compile_definitions( qa_spice
    PUBLIC EESCHEMA
)

kicad_add_boost_test( qa_spice qa_spice )

