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

find_package( Boost COMPONENTS unit_test_framework filesystem system REQUIRED )

set( QA_UTIL_COMMON_SRC
    stdstream_line_reader.cpp
    utility_program.cpp

    geometry/line_chain_construction.cpp
    geometry/poly_set_construction.cpp
    geometry/seg_construction.cpp

    wx_utils/unit_test_utils.cpp
    wx_utils/wx_assert.cpp
)

# A generic library of useful functions for various testing purposes
add_library( qa_utils STATIC
    ${QA_UTIL_COMMON_SRC}
)

include_directories( BEFORE ${INC_BEFORE} )

target_link_libraries( qa_utils
    common
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${wxWidgets_LIBRARIES}
)

target_include_directories( qa_utils PUBLIC
    include
)

target_compile_definitions( qa_utils PUBLIC
    BOOST_TEST_DYN_LINK
)
