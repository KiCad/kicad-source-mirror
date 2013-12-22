#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
#  Copyright (C) 2013 Kicad Developers, see AUTHORS.txt for contributors.
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



# Download a particular boost version, and patch it [and build it if BUILD_GITHUB_PLUGIN].
# Assumes include( ExternalProject ) was done inline previous to this file
# and that set( DOWNLOAD_DIR ... ) was set in a higher context.


#-----<configure>----------------------------------------------------------------

set( BOOST_RELEASE 1.54.0 )
set( BOOST_MD5 15cb8c0803064faef0c4ddf5bc5ca279 )   # re-calc this on every RELEASE change

# The boost headers [and static libs if built] go here, at the top of KiCad
# source tree in boost_root.
set( BOOST_ROOT "${PROJECT_SOURCE_DIR}/boost_root" )


# Space separated list which indicates the subset of boost libraries to compile.
# Chosen libraries are based on AVHTTP requirements, and possibly
# unit_test_framework for its own worth.
set( BOOST_LIBS_BUILT
    #context
    #coroutine
    date_time
    #exception
    filesystem
    iostreams
    locale
    program_options
    regex
    #signals
    system
    thread
    #unit_test_framework
    )

#-----</configure>---------------------------------------------------------------

find_package( BZip2 REQUIRED )

string( REGEX REPLACE "\\." "_" BOOST_VERS "${BOOST_RELEASE}" )
set( PREFIX ${DOWNLOAD_DIR}/boost_${BOOST_VERS} )

# <SOURCE_DIR> = ${PREFIX}/src/boost
# There is a Bazaar 'boost scratch repo' in <SOURCE_DIR>/boost and after committing pristine
# download, the patch is applied.  This lets you regenerate a new patch at any time
# easily, simply by editing the working tree in <SOURCE_DIR> and doing "bzr diff" in there.

# path to the boost headers in the repo.
# repo = "${headers_src}/../.bzr" = "<SOURCE_DIR>/.bzr"
set( headers_src "${PREFIX}/src/boost/boost" )


function( set_boost_lib_names libs output )
    foreach( lib ${libs} )
        set( fullpath_lib "${BOOST_ROOT}/lib/libboost_${lib}${CMAKE_STATIC_LIBRARY_SUFFIX}" )
        list( APPEND results ${fullpath_lib} )
    endforeach()
    # set the results into variable represented by output into caller's scope
    set( ${output} ${results} PARENT_SCOPE )
endfunction()


# (BTW "test" yields "unit_test_framework" when passed to bootstrap.sh ).
#message( STATUS "BOOST_LIBS_BUILT:${BOOST_LIBS_BUILT}" )
string( REPLACE "unit_test_framework" "test" boost_libs_list "${BOOST_LIBS_BUILT}" )
#message( STATUS "REPLACE libs_csv:${boost_libs_list}" )

if( MINGW )
    if( MSYS )
        # The Boost system does not build properly on MSYS using bootstrap.sh.  Running
        # bootstrap.bat with cmd.exe does.  It's ugly but it works.  At least for Boost
        # version 1.54.
        set( bootstrap cmd.exe /c "bootstrap.bat mingw" )
    else()
        set( bootstrap ./bootstrap.bat mingw )
    endif()

    foreach( lib ${boost_libs_list} )
        set( b2_libs ${b2_libs} --with-${lib} )
    endforeach()
    unset( PIC_STUFF )
else()
    string( REGEX REPLACE "\\;" "," libs_csv "${boost_libs_list}" )
    #message( STATUS "libs_csv:${libs_csv}" )

    set( bootstrap ./bootstrap.sh --with-libraries=${libs_csv} )
    # pass to *both* C and C++ compilers
    set( PIC_STUFF "cflags=${PIC_FLAG}" )
    set( BOOST_INCLUDE "${BOOST_ROOT}/include" )
    unset( b2_libs )
endif()

    set( TOOLSET "toolset=gcc" )

if( APPLE )
    # I set this to being compatible with wxWidgets
    # wxWidgets still using libstdc++ (gcc), meanwhile OSX
    # has switched to libc++ (llvm) by default
    set(BOOST_CXXFLAGS  "cxxflags=-mmacosx-version-min=10.5"  )
    set(BOOST_LINKFLAGS "linkflags=-mmacosx-version-min=10.5" )
    set( TOOLSET "" )

    if( CMAKE_OSX_ARCHITECTURES )

        if( (CMAKE_OSX_ARCHITECTURES MATCHES "386" OR CMAKE_OSX_ARCHITECTURES MATCHES "ppc ") AND
            (CMAKE_OSX_ARCHITECTURES MATCHES "64"))
            message("-- BOOST found 32/64 Address Model")

            set(BOOST_ADDRESSMODEL "address-model=32_64")
        endif()

        if( (${CMAKE_OSX_ARCHITECTURES} MATCHES "x86_64" OR ${CMAKE_OSX_ARCHITECTURES} MATCHES "386") AND
            (${CMAKE_OSX_ARCHITECTURES} MATCHES "ppc"))
            message("-- BOOST found ppc/intel Architecture")

            set(BOOST_ARCHITECTURE "architecture=combined")
        endif()

    endif()
endif()

ExternalProject_Add( boost
    PREFIX          "${PREFIX}"
    DOWNLOAD_DIR    "${DOWNLOAD_DIR}"
    INSTALL_DIR     "${BOOST_ROOT}"
    URL             http://downloads.sourceforge.net/project/boost/boost/${BOOST_RELEASE}/boost_${BOOST_VERS}.tar.bz2
    URL_MD5         ${BOOST_MD5}

    # The patch command executes with the working directory set to <SOURCE_DIR>
    # Revert the branch to pristine before applying patch sets as bzr patch
    # fails when applying a patch to the branch twice and doesn't have a switch
    # to ignore previously applied patches
    PATCH_COMMAND   bzr revert
        # PATCH_COMMAND continuation (any *_COMMAND here can be continued with COMMAND):
        COMMAND     bzr patch -p0 "${PROJECT_SOURCE_DIR}/patches/boost_minkowski.patch"
        COMMAND     bzr patch -p0 "${PROJECT_SOURCE_DIR}/patches/boost_cstdint.patch"

    # [Mis-]use this step to erase all the boost headers and libraries before
    # replacing them below.
    UPDATE_COMMAND  ${CMAKE_COMMAND} -E remove_directory "${BOOST_ROOT}"

    BINARY_DIR      "${PREFIX}/src/boost/"
    CONFIGURE_COMMAND ${bootstrap}

    BUILD_COMMAND   ./b2
                    variant=release
                    threading=multi
                    ${PIC_STUFF}
                    ${TOOLSET}
                    ${BOOST_CXXFLAGS}
                    ${BOOST_LINKFLAGS}
                    ${BOOST_ADDRESSMODEL}
                    ${BOOST_ARCHITECTURE}
                    ${b2_libs}
                    #link=static
                    --prefix=<INSTALL_DIR>
                    install

    INSTALL_COMMAND ""
    )

if( MINGW )
    execute_process( COMMAND ${CMAKE_C_COMPILER} -dumpversion
        OUTPUT_VARIABLE GCC_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE )

    string( REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.[0-9]+.*" "\\1\\2" BOOST_GCC_VERSION ${GCC_VERSION} )
    #message( STATUS "BOOST_GCC_VERSION: ${BOOST_GCC_VERSION}" )

    string( REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9])" "\\1_\\2" BOOST_LIB_VERSION ${BOOST_RELEASE} )
    #message( STATUS "BOOST_LIB_VERSION: ${BOOST_LIB_VERSION}" )

    # adjust the names of the libraries to suit the build. There's no
    # symbolic links provided on the MinGW build to allow us to use
    # generic names for the libs
    foreach( lib ${BOOST_LIBS_BUILT} )
        set( mingw_boost_libs ${mingw_boost_libs} ${lib}-mgw${BOOST_GCC_VERSION}-mt-${BOOST_LIB_VERSION} )
    endforeach()

    set( BOOST_LIBS_BUILT ${mingw_boost_libs} )
    set( BOOST_INCLUDE "${BOOST_ROOT}/include/boost-${BOOST_LIB_VERSION}" )
    unset( mingw_boost_libs )
endif()

set( boost_libs "" )
set_boost_lib_names( "${BOOST_LIBS_BUILT}" boost_libs )

set( Boost_LIBRARIES    ${boost_libs}      CACHE FILEPATH "Boost libraries directory" )
set( Boost_INCLUDE_DIR  "${BOOST_INCLUDE}" CACHE FILEPATH "Boost include directory" )

mark_as_advanced( Boost_LIBRARIES Boost_INCLUDE_DIR )

#message( STATUS "BOOST_ROOT:${BOOST_ROOT}  BOOST_LIBRARIES:${BOOST_LIBRARIES}" )
#message( STATUS "Boost_INCLUDE_DIR: ${Boost_INCLUDE_DIR}" )



ExternalProject_Add_Step( boost bzr_commit_boost
    COMMAND bzr ci -q -m pristine <SOURCE_DIR>
    COMMENT "committing pristine boost files to 'boost scratch repo'"
    DEPENDERS patch
    )


ExternalProject_Add_Step( boost bzr_add_boost
    # add only the headers to the scratch repo, repo = "../.bzr" from ${headers_src}
    COMMAND bzr add -q ${headers_src}
    COMMENT "adding pristine boost files to 'boost scratch repo'"
    DEPENDERS bzr_commit_boost
    )


ExternalProject_Add_Step( boost bzr_init_boost
    COMMAND bzr init -q <SOURCE_DIR>
    COMMENT "creating 'boost scratch repo' specifically for boost to track boost patches"
    DEPENDERS bzr_add_boost
    DEPENDEES download
    )

