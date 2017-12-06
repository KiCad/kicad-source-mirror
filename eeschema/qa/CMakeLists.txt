#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2017 CERN
# @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

find_package( Boost COMPONENTS unit_test_framework REQUIRED )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${INC_AFTER}
    )

add_executable( qa_eagle_plugin
    test_module.cpp
    test_basic.cpp
    )

target_compile_definitions( qa_eagle_plugin
    PRIVATE -DBOOST_TEST_DYN_LINK )

add_dependencies( qa_eagle_plugin common eeschema_kiface )

target_link_libraries( qa_eagle_plugin
    common
    eeschema_kiface
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${wxWidgets_LIBRARIES}
    )
