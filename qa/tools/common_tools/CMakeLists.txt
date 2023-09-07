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

add_executable( qa_common_tools

    # # stuff from common due to...units?
    # ../../common/eda_text.cpp

    # Mock Pgm needed for advanced_config in coroutines
    ${CMAKE_SOURCE_DIR}/qa/mocks/kicad/common_mocks.cpp

    # The main entry point
    main.cpp

    tools/coroutines/coroutines.cpp

    tools/io_benchmark/io_benchmark.cpp

    tools/sexpr_parser/sexpr_parse.cpp
)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/qa/mocks/include
    ${INC_AFTER}
)

target_link_libraries( qa_common_tools
    common
    core
    libcontext
    gal
    qa_utils
    sexpr
    ${wxWidgets_LIBRARIES}
)

# we need to pretend to be something to appease the units code
target_compile_definitions( qa_common_tools
    PRIVATE PCBNEW
)

kicad_add_utils_executable( qa_common_tools )
