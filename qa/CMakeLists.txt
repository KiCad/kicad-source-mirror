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
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# or you may search the http://www.gnu.org website for the version 2 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

include( KiCadQABuildUtils )

macro(setup_qa_env target_name)
    set( MACOS_BIN "" )
    if( APPLE AND ${target_name} STREQUAL "qa_cli" )
        set( MACOS_BIN "PATH=path_list_prepend:${CMAKE_INSTALL_PREFIX}/KiCad.app/Contents/MacOS" )
    endif()

    set( VCPKG_BIN "" )
    if( MSVC )
        if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
            set( VCPKG_BIN "PATH=path_list_prepend:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin" )
        else()
            set( VCPKG_BIN "PATH=path_list_prepend:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin" )
        endif()
    endif()

    # Once we move the cmake minimum to 3.22, we can use this for all platforms and not just MSVC
    # the old method only worked on MSVC due to a bug in cmake which was later fixed after ENVIRONMENT_MODIFICATION was added
    set( PATH_MODIFICATION
      "${VCPKG_BIN}"
      "${MACOS_BIN}"
      "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/kicad/"
      "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/common/"
      "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/api/"
      "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/common/gal/"
      "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/pcbnew/"
      "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/eeschema/"
      "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/cvpcb/" )

    set_tests_properties( ${target_name}
        PROPERTIES
            ENVIRONMENT_MODIFICATION "${PATH_MODIFICATION}"
    )

    get_test_property( ${target_name} ENVIRONMENT current_env )

    # We now add another variable that holds the same paths as above but explicitly so that python tests
    # can configure python using its os.add_dll_directory() function. because python wont read from PATH anymore.
    set( KICAD_BUILD_PATHS
    "KICAD_BUILD_PATHS=${CMAKE_BINARY_DIR}/kicad:${CMAKE_BINARY_DIR}/common:${CMAKE_BINARY_DIR}/api:${CMAKE_BINARY_DIR}/common/gal:${CMAKE_BINARY_DIR}/pcbnew:${CMAKE_BINARY_DIR}/eeschema:${CMAKE_BINARY_DIR}/cvpcb"
    )

    set( new_env "${current_env};${KICAD_BUILD_PATHS}" )
    set_tests_properties( ${target_name}
        PROPERTIES
            ENVIRONMENT "${new_env}"
    )
endmacro()

# Shared QA helper libraries
add_subdirectory( qa_utils )
add_subdirectory( pcbnew_utils )
add_subdirectory( schematic_utils )

# Unit tests
add_subdirectory( tests )

# Utility/debugging/profiling programs
add_subdirectory( tools )

if( KICAD_SIGNAL_INTEGRITY )
    #add_subdirectory( ibis )
endif()
