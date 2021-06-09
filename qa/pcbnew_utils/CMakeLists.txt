# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

# Pcbnew-related auxiliary functions that are useful for QA purposes
# (both unit tests and utility programs)

# This is a massive hack in the CMake as I have no idea how to get this to
# link against pcbnew/pcbcommon/etc and still allow a dependency to also link
# THis should really be an add_library() and all the variables instead set using
# target_*() functions.

set( QA_PCBNEW_UTILS_SRCS

    ${CMAKE_CURRENT_SOURCE_DIR}/board_construction_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/board_file_utils.cpp
)

add_library( qa_pcbnew_utils STATIC ${QA_PCBNEW_UTILS_SRCS} )


target_include_directories( qa_pcbnew_utils PUBLIC BEFORE ${INC_BEFORE} )

target_include_directories( qa_pcbnew_utils PUBLIC
    include

    # Paths for pcbnew lib usage (should really be in pcbnew/common
    # target_include_directories and made PUBLIC)
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/pcbnew/router
    ${CMAKE_SOURCE_DIR}/pcbnew/tools
    ${CMAKE_SOURCE_DIR}/pcbnew/dialogs
    ${INC_AFTER}
)

add_dependencies( qa_pcbnew_utils
    pcbcommon
)

target_link_libraries( qa_pcbnew_utils PUBLIC
    qa_utils
)
#     3d-viewer
#     connectivity
#     pcbcommon
#     pnsrouter
#     common
#     pcbcommon
#     gal
#     qa_utils
#     lib_dxf
#     idf3
#     unit_test_utils
#     ${PCBNEW_IO_LIBRARIES}
#     ${wxWidgets_LIBRARIES}
#     ${GDI_PLUS_LIBRARIES}
#     ${PYTHON_LIBRARIES}
#     ${Boost_LIBRARIES}      # must follow GITHUB
#     ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
#)

# # we need to pretend to be something to appease the units code
target_compile_definitions( qa_pcbnew_utils
    PUBLIC PCBNEW
)
