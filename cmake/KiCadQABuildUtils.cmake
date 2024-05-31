# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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
# Utility CMake functions and targets for marshalling the builds of QA
# tests and tools
#

# CI tools can use this to force tests to output machine-readable formats
# when possible
option( KICAD_TEST_XML_OUTPUT
    "Cause unit tests to output xUnit results where possible for more granular CI reporting."
    OFF
)

mark_as_advanced( KICAD_TEST_XML_OUTPUT ) # Only CI tools need this

option( KICAD_TEST_DATABASE_LIBRARIES
    "Enable the database libraries QA tests (requires SQlite3 ODBC driver to be installed"
    OFF
)

# This is a "meta" target that is used to collect all tests
add_custom_target( qa_all_tests )

# This is a "meta" target used to collect all utility tools
add_custom_target( qa_all_tools )

# This "meta" target builds all QA tools, utils, etc
add_custom_target( qa_all
    DEPENDS qa_all_tests qa_all_tools
)

# Add a target as a "QA test" executable:
#   * Added as a CTest test with the given name
#   * Excluded from ALL when KICAD_BUILD_QA_TESTS not set
#   * Is a dependency of qa_all_tests
function( kicad_add_boost_test TEST_EXEC_TARGET TEST_NAME)

set(BOOST_TEST_PARAMS "")

if( KICAD_TEST_XML_OUTPUT )
    # Provide Boost-test-y XML params if asked
    # Due to Boost issue in 1.62, have to use the --logger parameter, rather than
    # separate --log_format, --log_sink, etc parameter
    # https://svn.boost.org/trac10/ticket/12507
    set(BOOST_TEST_PARAMS --logger=JUNIT,warning,${TEST_NAME}.boost-results.xml:HRF,message)
endif()

# Add the test to the CTest registry
add_test( NAME ${TEST_NAME}
    COMMAND $<TARGET_FILE:${TEST_EXEC_TARGET}> ${BOOST_TEST_PARAMS}
)

# Make the overall test meta-target depend on this test
add_dependencies( qa_all_tests ${TEST_EXEC_TARGET} )

# If tests are not enabled by default, remove from the ALL target
# They can still be built manually, or all together with the qa_all_test target
if( NOT KICAD_BUILD_QA_TESTS )

    set_target_properties( ${TEST_EXEC_TARGET}
        PROPERTIES EXCLUDE_FROM_ALL TRUE
    )

endif()

endfunction()

# Add a target as a "QA tool" executable:
#   * Excluded from ALL when KICAD_BUILD_QA_TESTS not set
#   * Is a dependency of qa_all_tools
function( kicad_add_utils_executable TARGET)

# If tests are not enabled by default, remove from the ALL target
# They can still be built manually, or all together with the qa_all_tools
if( NOT KICAD_BUILD_QA_TESTS )

    set_target_properties( ${TARGET}
        PROPERTIES EXCLUDE_FROM_ALL TRUE
    )

endif()

# Make the overall test meta-target depend on this test
add_dependencies( qa_all_tools ${TARGET} )

endfunction()