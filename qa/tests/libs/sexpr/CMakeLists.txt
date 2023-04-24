# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2019 KiCad Developers, see AUTHORS.TXT for contributors.
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

#
# Unit tests for s-expr handling routines

set( SEXPR_SRCS
    test_module.cpp

    test_sexpr.cpp
    test_sexpr_parser.cpp
)

add_executable( qa_sexpr ${SEXPR_SRCS} )

target_link_libraries( qa_sexpr
    sexpr
    qa_utils
    Boost::headers
    Boost::unit_test_framework
    ${wxWidgets_LIBRARIES}
)

target_include_directories( qa_sexpr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

kicad_add_boost_test(qa_sexpr qa_sexpr)
