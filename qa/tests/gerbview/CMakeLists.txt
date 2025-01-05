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


set( QA_GERBVIEW_SRCS
    # The main test entry points
    test_module.cpp

    # Shared between programs, but dependent on the BIU
    ${CMAKE_SOURCE_DIR}/qa/tests/common/test_format_units.cpp
)

if( WIN32 )
    # We want to declare a resource manifest on Windows to enable UTF8 mode
    # Without UTF8 mode, some random IO tests may fail, we set the active code page on normal kicad to UTF8 as well
    if( MINGW )
        # QA_GERBVIEW_RESOURCES variable is set by the macro.
        mingw_resource_compiler( qa_gerbview )
    else()
        set( QA_GERBVIEW_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/qa_gerbview.rc )
    endif()
endif()

add_executable( qa_gerbview
    ${QA_GERBVIEW_SRCS}
    ${QA_GERBVIEW_RESOURCES}
)

# Gerbview tests, so pretend to be gerbview (for units, etc)
target_compile_definitions( qa_gerbview
    PRIVATE GERBVIEW
)

target_include_directories( qa_gerbview PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/qa/mocks/include
)

# Anytime we link to the kiface_objects, we have to add a dependency on the last object
# to ensure that the generated lexer files are finished being used before the qa runs in a
# multi-threaded build
add_dependencies( qa_gerbview gerbview )

target_link_libraries( qa_gerbview
    gerbview_kiface_objects
    pcbcommon
    gal
    common
    gal
    qa_utils
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${PYTHON_LIBRARIES}
    Boost::headers
    Boost::unit_test_framework
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
)

kicad_add_boost_test( qa_gerbview qa_gerbview )

setup_qa_env( qa_gerbview )