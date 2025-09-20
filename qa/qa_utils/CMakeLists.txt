# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

set( QA_UTIL_COMMON_SRC
    stdstream_line_reader.cpp
    utility_program.cpp
    error_handlers.cpp

    uuid_test_utils.cpp

    geometry/line_chain_construction.cpp
    geometry/poly_set_construction.cpp
    geometry/seg_construction.cpp

    wx_utils/unit_test_utils.cpp
    wx_utils/wx_assert.cpp

    # PDF testing helpers
    pdf_test_utils.cpp
)

# A generic library of useful functions for various testing purposes
add_library( qa_utils STATIC
    ${QA_UTIL_COMMON_SRC}
)

include_directories( BEFORE ${INC_BEFORE} )

# for some obscure reason, on mingw/msys2 Boost::unit_test_framework does not work
# So use the actual lib filename:
if( MINGW )
    set( LIBBOOST_UNIT_TEST_LIB libboost_unit_test_framework-mt )
endif()

target_link_libraries( qa_utils
    common
    turtle
    Boost::headers
    Boost::unit_test_framework
    ${wxWidgets_LIBRARIES}
    ${LIBBOOST_UNIT_TEST_LIB}
)

# Pass in the default data location
set_source_files_properties( wx_utils/unit_test_utils.cpp PROPERTIES
    COMPILE_DEFINITIONS "QA_DATA_ROOT=(\"${CMAKE_SOURCE_DIR}/qa/data\")"
)

target_include_directories( qa_utils PUBLIC
    include
)

target_compile_definitions( qa_utils PUBLIC
    BOOST_TEST_DYN_LINK
)
