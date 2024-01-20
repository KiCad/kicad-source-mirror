# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2023 KiCad Developers, see AUTHORS.TXT for contributors.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.

set( QA_API_SRCS
    test_api_module.cpp
    test_api_enums.cpp
    test_api_proto.cpp
    )

add_executable( qa_api
    ${QA_API_SRCS}
    )

add_dependencies( qa_api pcbnew )

target_link_libraries( qa_api
    kiapi
    qa_utils
    qa_pcbnew_utils
    pcbnew_kiface_objects
    3d-viewer
    connectivity
    pcbcommon
    pnsrouter
    gal
    common
    idf3
    markdown_lib
    ${PCBNEW_IO_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    Boost::headers
    Boost::unit_test_framework
    ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
    )

target_include_directories( qa_api PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/qa/mocks/include
    ${CMAKE_SOURCE_DIR}/pcbnew
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_PROPERTY:magic_enum,INTERFACE_INCLUDE_DIRECTORIES>
    )

kicad_add_boost_test( qa_api qa_api )
