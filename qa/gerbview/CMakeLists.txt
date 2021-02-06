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
    ${CMAKE_SOURCE_DIR}/qa/common/test_format_units.cpp
)

add_executable( qa_gerbview
    ${QA_GERBVIEW_SRCS}

    # In order to enable this, the actual Gerbview kiface build must be updated to match pcbnew's
    # Older CMakes cannot link OBJECT libraries
    # https://cmake.org/pipermail/cmake/2013-November/056263.html
    $<TARGET_OBJECTS:gerbview_kiface_objects>
)

# Gerbview tests, so pretend to be gerbview (for units, etc)
target_compile_definitions( qa_gerbview
    PRIVATE GERBVIEW
)

target_include_directories( qa_gerbview PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Anytime we link to the kiface_objects, we have to add a dependency on the last object
# to ensure that the generated lexer files are finished being used before the qa runs in a
# multi-threaded build
add_dependencies( qa_gerbview gerbview )

target_link_libraries( qa_gerbview
    pcbcommon
    gal
    common
    gal
    qa_utils
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${Boost_LIBRARIES}      # must follow GITHUB
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
)

kicad_add_boost_test( qa_gerbview qa_gerbview )
