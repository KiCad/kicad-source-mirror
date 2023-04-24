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

find_package(Boost COMPONENTS unit_test_framework REQUIRED)
find_package( wxWidgets 3.0.0 COMPONENTS gl aui adv html core net base xml stc REQUIRED )


add_compile_definitions( BOOST_TEST_DYN_LINK EESCHEMA DRC_PROTO TEST_APP_NO_MAIN)

add_executable( ibis_proto
    qaIbisParser.cpp
    ${CMAKE_SOURCE_DIR}/eeschema/sim/kibis/ibis_parser.cpp
    ${CMAKE_SOURCE_DIR}/eeschema/sim/kibis/kibis.cpp
    ${CMAKE_SOURCE_DIR}/eeschema/sim/ngspice_circuit_model.cpp
    ${CMAKE_SOURCE_DIR}/eeschema/sim/ngspice.cpp
)

add_dependencies( ibis_proto pnsrouter pcbcommon )

include_directories( BEFORE ${INC_BEFORE} )
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/eeschema
    ${CMAKE_SOURCE_DIR}/eeschema/sim
    ${CMAKE_SOURCE_DIR}/qa/eeschema_utils/include
    ${INC_AFTER}
)

target_link_libraries( ibis_proto
    qa_eechema_utils
    3d-viewer
    connectivity
    pcbcommon
    pnsrouter
    gal
    common
    gal
    qa_utils
    dxflib_qcad
    tinyspline_lib
    nanosvg
    idf3
    Boost::headers
    ${wxWidgets_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${EESCHEMA_EXTRA_LIBS}    # -lrt must follow Boost
)

configure_file( "ibis_v1_1.ibs" "ibis_v1_1.ibs" COPYONLY )
configure_file( "ibis_v2_1.ibs" "ibis_v2_1.ibs" COPYONLY )
configure_file( "ibis_v2_1.pkg" "ibis_v2_1.pkg" COPYONLY )
