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

set( QA_KINNG_SRCS
    kinng_test_module.cpp
    test_kinng.cpp
    )

add_executable( qa_kinng
    ${QA_KINNG_SRCS}
    )

target_link_libraries( qa_kinng
    qa_utils
    kinng
    kiapi
    ${NNG_LIBRARY}
    ${wxWidgets_LIBRARIES}
    )

target_include_directories( qa_kinng PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/qa/mocks/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

kicad_add_boost_test( qa_kinng qa_kinng )
