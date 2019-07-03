
#  This program source code file is part of KICAD, a free EDA CAD application.
#
# Copyright (C) 2011 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
# Copyright (C) 2011 Kicad Developers, see change_log.txt for contributors#.
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

# for now, this is a stand alone project
if( 1 )

    project(kicad-new)

    cmake_minimum_required( VERSION 2.8 FATAL_ERROR )

    set( PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ )

    # message( "PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}" )

    # Path to local CMake modules.
    set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules )

    include( CheckFindPackageResult )

    find_package(Doxygen)

    # On Apple only wxwidgets 2.9 or higher doesn't need to find aui part of base
    if(APPLE)
        find_package(wxWidgets COMPONENTS gl adv html core net base xml QUIET )
    else()
        find_package(wxWidgets COMPONENTS gl adv html core net base xml aui QUIET )
    endif()

    check_find_package_result( wxWidgets_FOUND "wxWidgets" )

    # make config.h
    include( PerformFeatureChecks )
    perform_feature_checks()

    # Include wxWidgets macros.
    include( ${wxWidgets_USE_FILE} )

    include_directories( ${PROJECT_SOURCE_DIR}/include )

    if(CMAKE_COMPILER_IS_GNUCXX)
        # Set default flags for Release build.
        set(CMAKE_C_FLAGS_RELEASE "-Wall -O2 -DNDEBUG ")
        set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O2 -DNDEBUG")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s -static-libgcc")

        # Set default flags for Debug build.
        set(CMAKE_C_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")
        set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")
    endif(CMAKE_COMPILER_IS_GNUCXX)

    include(Functions)

endif()


#================================================
# Doxygen Output
#================================================
if(DOXYGEN_FOUND)
    add_custom_target( new-docs ${DOXYGEN_EXECUTABLE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS Doxyfile )
else(DOXYGEN_FOUND)
    message( STATUS "WARNING: Doxygen not found - new-docs (Source Docs) target not created" )
endif()


include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories( ${CMAKE_BINARY_DIR} )

#=====<on standby for possible C++ unit testing>================================
if( false )

    add_executable( test_dir_lib_source
        sch_dir_lib_source.cpp
        )
    target_link_libraries( test_dir_lib_source ${wxWidgets_LIBRARIES} )

    add_executable( test_sch_part sch_part.cpp )
    target_link_libraries( test_sch_part ${wxWidgets_LIBRARIES} )

    add_executable( test_lpid
        sch_lpid.cpp
        )
    target_link_libraries( test_lpid ${wxWidgets_LIBRARIES} )

endif( false )

#=====</on standby for possible C++ unit testing>===============================


add_library( sweet SHARED
    sch_lib_table.cpp
    sch_lib.cpp
    sch_lpid.cpp
    sch_dir_lib_source.cpp
    sch_part.cpp
    sch_sweet_parser.cpp
    ${PROJECT_SOURCE_DIR}/common/richio.cpp
    ${PROJECT_SOURCE_DIR}/common/dsnlexer.cpp
    )

make_lexer(
    sweet
    ${CMAKE_CURRENT_SOURCE_DIR}/sch_lib_table.keywords
    ${CMAKE_CURRENT_SOURCE_DIR}/sch_lib_table_lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sch_lib_table_keywords.cpp
    LT
    )

make_lexer(
    sweet
    ${CMAKE_CURRENT_SOURCE_DIR}/sweet.keywords
    ${CMAKE_CURRENT_SOURCE_DIR}/sweet_lexer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/sweet_keywords.cpp
    PR
    )

target_link_libraries( sweet ${wxWidgets_LIBRARIES} )

# talk to import_export.h
set_target_properties( sweet PROPERTIES
    DEFINE_SYMBOL COMPILING_DLL
    )

add_executable( test_sch_lib_table test_sch_lib_table.cpp )
if( MINGW )
    set_target_properties( test_sch_lib_table PROPERTIES
        LINK_FLAGS -Wl,--enable-auto-import
    )
endif()
target_link_libraries( test_sch_lib_table sweet )


if( 1 )

    find_library( gal_LIBRARY "libgal.a"
        PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/../../kicad-gal/build"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../kicad-gal/build"
        DOC
            "Where is the static GAL library"
        )
    find_path( gal_ROOT NAMES "README.txt"
        PATHS
            "${CMAKE_CURRENT_SOURCE_DIR}/../../kicad-gal"
            "${CMAKE_CURRENT_SOURCE_DIR}/../../../kicad-gal"
        DOC
            "Where is the base include directory for the GAL library"
        )

else()

    # we build the GAL library, therefore we know where it is, no need to find_library() it.
    set( gal_ROOT    "${CMAKE_CURRENT_BINARY_DIR}/gal-src" )
    set( gal_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/gal-src/libgal.a" )

    include( ExternalProject )
    ExternalProject_Add( ki_gal
        # skip the install step, build in source, use from source
        SOURCE_DIR "${gal_ROOT}"
        STAMP_DIR  "${CMAKE_CURRENT_BINARY_DIR}/gal-stamps"
        TIMEOUT 120
        DOWNLOAD_COMMAND bzr export ${gal_ROOT} lp:~kicad-testing-committers/kicad/kicad-gal
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${gal_ROOT}
        BUILD_IN_SOURCE 1
        BUILD_COMMAND make
        INSTALL_COMMAND ""
        )
endif()

include_directories( "${gal_ROOT}" )

# Find pkg-config in order find cairo.
find_package( PkgConfig REQUIRED )

# Find cairo.
#pkg_search_module( CAIRO REQUIRED cairo>=1.8.1 )
#include_directories( ${CAIRO_INCLUDE_DIRS} )


# Find the OpenGL libraries (gl and glu)
find_package( OpenGL )


# Find GLEW
pkg_search_module( GLEW REQUIRED glew>=1.5.0 )
include_directories( ${GLEW_INCLUDE_DIRS} )


include_directories( ${gal_ROOT} )

add_executable( sweet_edit sweet_edit.cpp  sweet_editor_panel.cpp sch_canvas.cpp )

target_link_libraries( sweet_edit
    sweet
    ${OPENGL_LIBRARIES}
    ${gal_LIBRARY}
    ${wxWidgets_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
    )


#=====<USE_SWIG>============================================================
# if you want a Python scripting layer on top for unit testing or driving.
# reference: http://www.swig.org/Doc1.3/Introduction.html#Introduction_build_system

option( USE_SWIG "Use SWIG to build a python scripting interface (default OFF)." OFF)
if( USE_SWIG )

    find_package( SWIG REQUIRED )
    include( ${SWIG_USE_FILE} )

    # first install "python-dev" linux package for this:
    find_package( PythonLibs )
    include_directories( ${PYTHON_INCLUDE_PATH} )

    set( CMAKE_SWIG_FLAGS "" )

    set_source_files_properties( sweet.i PROPERTIES CPLUSPLUS ON )
    #set_source_files_properties( sweet.i PROPERTIES SWIG_FLAGS "-includeall" )

    swig_add_module( sweet python sweet.i )
    swig_link_libraries( sweet ${PYTHON_LIBRARIES} sweet )

    # @todo need a separate interface for each DSNLEXER

endif()

