# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2014-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

## Unit tests
add_subdirectory( api )
add_subdirectory( common )
add_subdirectory( diff_merge )
add_subdirectory( gerbview )
add_subdirectory( eeschema )
add_subdirectory( libs )
add_subdirectory( pcbnew )

if( KICAD_SPICE_QA )
    add_subdirectory( spice )
endif()

if( KICAD_TEST_XML_OUTPUT )
    # To do this, you will need xmlrunner
    set( PYTEST_ARGS_QAPYTHON --junitxml=${CMAKE_CURRENT_BINARY_DIR}/qapython.junit-results.xml )
    set( PYTEST_ARGS_QACLI --junitxml=${CMAKE_CURRENT_BINARY_DIR}/qacli.junit-results.xml )
endif()

set( EXTRA_PYTEST_ENVIRONMENT "" )

if( APPLE )
    # We need to use the installed kicad-cli not the build dir to run these tests
    get_filename_component( CAIRO_LIB_BASE "${CAIRO_LIBRARY}" DIRECTORY )
    set( EXTRA_PYTEST_ENVIRONMENT "DYLD_FALLBACK_LIBRARY_PATH=${CAIRO_LIB_BASE}" )

elseif ( MSVC )
    set( EXTRA_PYTEST_ENVIRONMENT "FONTCONFIG_PATH=${CMAKE_SOURCE_DIR}/qa/resources/windows/fonts/" )
endif()

# Use pytest-xdist for parallel CLI test execution when available
execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import xdist"
    RESULT_VARIABLE PYTEST_XDIST_RESULT
    OUTPUT_QUIET
    ERROR_QUIET
)

if( PYTEST_XDIST_RESULT EQUAL 0 )
    set( PYTEST_PARALLEL_ARGS -n auto )
    message( STATUS "pytest-xdist found, qa_cli tests will run in parallel" )
else()
    set( PYTEST_PARALLEL_ARGS "" )
    message( STATUS "pytest-xdist not found, qa_cli tests will run sequentially" )
endif()

add_test(NAME qa_cli
    COMMAND ${PYTHON_EXECUTABLE} -m pytest ${PYTEST_PARALLEL_ARGS}
            ${CMAKE_CURRENT_SOURCE_DIR}/cli ${PYTEST_ARGS_QACLI}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set_tests_properties( qa_cli
    PROPERTIES
        ENVIRONMENT "KICAD_CLI=$<TARGET_FILE:kicad-cli>;PYTHONPATH=${CMAKE_BINARY_DIR}/pcbnew;${PYTHON_QA_PATH};${EXTRA_PYTEST_ENVIRONMENT}"
)

setup_qa_env( qa_cli )
