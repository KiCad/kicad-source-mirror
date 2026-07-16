# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Drives the backend runtime's server in process
add_executable( qa_mdw_runtime
    test_mdw_backend_server.cpp
    test_mdw_event_publisher.cpp
    test_mdw_file_io.cpp
    test_mdw_poller.cpp
    )

target_link_libraries( qa_mdw_runtime
    mdw_runtime
    Boost::headers
    Boost::unit_test_framework
    )

target_include_directories( qa_mdw_runtime PRIVATE
    ${NNG_INCLUDE_DIR}
    )

target_compile_definitions( qa_mdw_runtime PRIVATE
    BOOST_TEST_DYN_LINK
    )

kicad_add_boost_test( qa_mdw_runtime qa_mdw_runtime NO_CATCH_SYSTEM_ERRORS TIMEOUT 300 )

setup_qa_env( qa_mdw_runtime )
