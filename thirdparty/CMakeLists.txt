#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2007-2020 Kicad Developers, see AUTHORS.txt for contributors.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, you may find one here:
#  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
#  or you may search the http://www.gnu.org website for the version 2 license,
#  or you may write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#

# Disable shared library building for all thirdparty libraries
set( BUILD_SHARED_LIBS OFF )

# Note: The glew folder isn't added here because it is added inside the main CMakeLists.txt

# If this is not set, then we cannot set things like ARGPARSE_INSTALL to OFF before the
# argparse subdirectory is added.  Under the new policy, add_option will allow itself to
# be overridden by a previously-set variable like we want to do here.  Doing it this way
# instead of with cmake_policy will apply the change to the third-party CMakeLists files.
set( CMAKE_POLICY_DEFAULT_CMP0077 NEW )

# Avoid spurious warnings on g++
if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    set_source_files_properties(
        ${CMAKE_SOURCE_DIR}/thirdparty/3dxware_sdk/src/navlib_stub.c
        PROPERTIES COMPILE_FLAGS "-Wno-attributes"
    )
endif()

set( ARGPARSE_INSTALL OFF )
add_subdirectory( argparse )
add_subdirectory( clipper2 )
add_subdirectory( compoundfilereader )
add_subdirectory( delaunator )
add_subdirectory( dxflib_qcad )
add_subdirectory( dynamic_bitset )
add_subdirectory( expected )
set( FASTFLOAT_INSTALL OFF )
add_subdirectory( fast_float )
set( FMT_INSTALL OFF )
add_subdirectory( fmt )
add_subdirectory( gzip-hpp )
add_subdirectory( lemon )
add_subdirectory( libcontext )
add_subdirectory( libpopcnt )
add_subdirectory( magic_enum )
add_subdirectory( markdown2html )
add_subdirectory( nanoflann )
add_subdirectory( nanodbc )
add_subdirectory( nanosvg )
add_subdirectory( rectpack2d )
add_subdirectory( rtree )
add_subdirectory( tinyspline_lib )
add_subdirectory( potrace )
add_subdirectory( nlohmann_json )
add_subdirectory( picosha2 )
add_subdirectory( json_schema_validator )
add_subdirectory( pegtl )
add_subdirectory( 3dxware_sdk )
add_subdirectory( rapidcsv )
add_subdirectory( turtle )
add_subdirectory( thread-pool )
add_subdirectory( zint )
