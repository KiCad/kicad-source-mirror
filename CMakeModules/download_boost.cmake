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
# tool_manager.cpp -> coroutine -> context (_jump_fcontext) (on OSX)

set( BOOST_LIBS_BUILT
    context
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

if( NOT BZIP2_FOUND )
    find_package( BZip2 REQUIRED )
endif()

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

# Default Toolset
set( BOOST_TOOLSET "toolset=gcc" )

if( KICAD_BUILD_STATIC OR APPLE )
    set( BOOST_LINKTYPE  "link=static" )
else()
    unset( BOOST_LINKTYPE )
endif()


find_program(patch_bin NAMES patch patch.exe)

if( "${patch_bin}" STREQUAL "patch_bin-NOTFOUND" )
    set( PATCH_STR_CMD ${PATCH_STR_CMD} )
else()
    set( PATCH_STR_CMD ${patch_bin} -p0 -i )
endif()



if( MINGW AND NOT CMAKE_HOST_UNIX )  # building for MINGW on windows not UNIX
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
    unset( BOOST_CFLAGS )

else()
    string( REGEX REPLACE "\\;" "," libs_csv "${boost_libs_list}" )
    #message( STATUS "libs_csv:${libs_csv}" )

    set( bootstrap ./bootstrap.sh --with-libraries=${libs_csv} )
    # pass to *both* C and C++ compilers
    set( BOOST_CFLAGS   "cflags=${PIC_FLAG}" )
    set( BOOST_CXXFLAGS "cxxflags=${PIC_FLAG}" )
    set( BOOST_INCLUDE  "${BOOST_ROOT}/include" )
    unset( b2_libs )
endif()


if( APPLE )
    set( BOOST_CXXFLAGS  "cxxflags=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} -fno-common" )
    set( BOOST_LINKFLAGS "linkflags=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} -fno-common" )
    set( BOOST_TOOLSET   "toolset=darwin" )

    if( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
        set(BOOST_CXXFLAGS  "${BOOST_CXXFLAGS} -fno-lto" )
        set(BOOST_LINKFLAGS "${BOOST_LINKFLAGS} -fno-lto" )
    endif()

    if( CMAKE_OSX_ARCHITECTURES )

        if( (CMAKE_OSX_ARCHITECTURES MATCHES "386" OR CMAKE_OSX_ARCHITECTURES MATCHES "ppc ") AND
            (CMAKE_OSX_ARCHITECTURES MATCHES "64"))
            message( "-- BOOST found 32/64 Address Model" )

            set( BOOST_ADDRESSMODEL "address-model=32_64" )
        endif()

        if( (CMAKE_OSX_ARCHITECTURES MATCHES "x86_64" OR CMAKE_OSX_ARCHITECTURES MATCHES "386") AND
            (CMAKE_OSX_ARCHITECTURES MATCHES "ppc"))
            message("-- BOOST found ppc/x86 Architecture")

            set(BOOST_ARCHITECTURE "architecture=combined")
        elseif( (CMAKE_OSX_ARCHITECTURES MATCHES "x86_64" OR CMAKE_OSX_ARCHITECTURES MATCHES "386") )
            message("-- BOOST found x86 Architecture")

            set(BOOST_ARCHITECTURE "architecture=x86")
        elseif( (CMAKE_OSX_ARCHITECTURES MATCHES "ppc64" OR CMAKE_OSX_ARCHITECTURES MATCHES "ppc") )
            message("-- BOOST found ppc Architecture")

            set(BOOST_ARCHITECTURE "architecture=ppc")
        endif()

        set( BOOST_CFLAGS    "${BOOST_CFLAGS} -arch ${CMAKE_OSX_ARCHITECTURES}"  )
        set( BOOST_CXXFLAGS  "${BOOST_CXXFLAGS} -arch ${CMAKE_OSX_ARCHITECTURES}"  )
        set( BOOST_LINKFLAGS "${BOOST_LINKFLAGS} -arch ${CMAKE_OSX_ARCHITECTURES}" )
    endif()
endif()

ExternalProject_Add( boost
    PREFIX          "${PREFIX}"

    URL             http://downloads.sourceforge.net/project/boost/boost/${BOOST_RELEASE}/boost_${BOOST_VERS}.tar.bz2
    DOWNLOAD_DIR    "${DOWNLOAD_DIR}"
    TIMEOUT         1200            # 20 minutes
    URL_MD5         ${BOOST_MD5}
    # If download fails, then enable "LOG_DOWNLOAD ON" and try again.
    # Upon a second failure with logging enabled, then look at these logs:
    # <src>/.downloads-by-cmake$ less /tmp/product/.downloads-by-cmake/boost_1_54_0/src/boost-stamp/boost-download-out.log
    # <src>/.downloads-by-cmake$ less /tmp/product/.downloads-by-cmake/boost_1_54_0/src/boost-stamp/boost-download-err.log
    # If out.log does not show 100%, then try increasing TIMEOUT even more, or download the URL manually and put it
    # into <src>/.downloads-by-cmake/ dir.
 #  LOG_DOWNLOAD    ON

    INSTALL_DIR     "${BOOST_ROOT}"

    # The patch command executes with the working directory set to <SOURCE_DIR>
    # Revert the branch to pristine before applying patch sets as bzr patch
    # fails when applying a patch to the branch twice and doesn't have a switch
    # to ignore previously applied patches
    PATCH_COMMAND   bzr revert
        # bzr revert is insufficient to remove "added" files:
        COMMAND     bzr clean-tree -q --force

        COMMAND     ${PATCH_STR_CMD} "${PROJECT_SOURCE_DIR}/patches/boost_minkowski.patch"
        COMMAND     ${PATCH_STR_CMD} "${PROJECT_SOURCE_DIR}/patches/boost_cstdint.patch"

        COMMAND     ${PATCH_STR_CMD} "${PROJECT_SOURCE_DIR}/patches/boost_macosx_x86.patch"        #https://svn.boost.org/trac/boost/ticket/8266
        # tell bzr about "added" files by last patch:
        COMMAND     bzr add libs/context/src/asm/jump_i386_x86_64_sysv_macho_gas.S
        COMMAND     bzr add libs/context/src/asm/make_i386_x86_64_sysv_macho_gas.S

        COMMAND     ${PATCH_STR_CMD} "${PROJECT_SOURCE_DIR}/patches/boost_macosx_x86_build.patch"  #https://svn.boost.org/trac/boost/ticket/8266
        COMMAND     ${PATCH_STR_CMD} "${PROJECT_SOURCE_DIR}/patches/boost_macosx_older_openssl.patch"  #https://svn.boost.org/trac/boost/ticket/9273

        COMMAND     ${PATCH_STR_CMD} "${PROJECT_SOURCE_DIR}/patches/boost_mingw.patch"             #https://svn.boost.org/trac/boost/ticket/7262
        COMMAND     ${PATCH_STR_CMD} "${PROJECT_SOURCE_DIR}/patches/boost_mingw64_interlocked.patch"

        # tell bzr about "added" files by last patch:
        COMMAND     bzr add libs/context/src/asm/make_i386_ms_pe_gas.S
        COMMAND     bzr add libs/context/src/asm/jump_i386_ms_pe_gas.S
        COMMAND     bzr add libs/context/src/asm/make_x86_64_ms_pe_gas.S
        COMMAND     bzr add libs/context/src/asm/jump_x86_64_ms_pe_gas.S

        COMMAND     ${PATCH_STR_CMD} "${PROJECT_SOURCE_DIR}/patches/patch_macosx_context_ppc_v2.patch" #https://svn.boost.org/trac/boost/ticket/8266
        COMMAND     bzr add libs/context/build/Jamfile.v2
        COMMAND     bzr add libs/context/build/architecture.jam
        COMMAND     bzr add libs/context/src/asm/jump_combined_sysv_macho_gas.S
        COMMAND     bzr add libs/context/src/asm/jump_ppc32_sysv_macho_gas.S
        COMMAND     bzr add libs/context/src/asm/jump_ppc64_sysv_macho_gas.S
        COMMAND     bzr add libs/context/src/asm/make_combined_sysv_macho_gas.S
        COMMAND     bzr add libs/context/src/asm/make_ppc32_sysv_macho_gas.S
        COMMAND     bzr add libs/context/src/asm/make_ppc64_sysv_macho_gas.S

    # [Mis-]use this step to erase all the boost headers and libraries before
    # replacing them below.
    UPDATE_COMMAND  ${CMAKE_COMMAND} -E remove_directory "${BOOST_ROOT}"

    BINARY_DIR      "${PREFIX}/src/boost/"
    CONFIGURE_COMMAND ${bootstrap}

    BUILD_COMMAND   ./b2
                    variant=release
                    threading=multi
                    ${BOOST_CFLAGS}
                    ${BOOST_TOOLSET}
                    ${BOOST_CXXFLAGS}
                    ${BOOST_LINKFLAGS}
                    ${BOOST_ADDRESSMODEL}
                    ${BOOST_ARCHITECTURE}
                    ${b2_libs}
                    ${BOOST_LINKTYPE}
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

set( Boost_LIBRARIES    ${boost_libs} )
set( Boost_INCLUDE_DIR  "${BOOST_INCLUDE}" )

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
    COMMAND bzr add -q ${PREFIX}/src/boost
    COMMENT "adding pristine boost files to 'boost scratch repo'"
    DEPENDERS bzr_commit_boost
    )


ExternalProject_Add_Step( boost bzr_init_boost
    COMMAND bzr init -q <SOURCE_DIR>
    #creates a .bzrignore file in boost root dir, to avoid copying useless files
    #moreover these files have a very very long name, and sometimes
    #have a too long full file name to be handled by DOS commands
    COMMAND echo "*.htm*" > ${PREFIX}/src/boost/.bzrignore
    COMMENT "creating 'boost scratch repo' specifically for boost to track boost patches"
    DEPENDERS bzr_add_boost
    DEPENDEES download
    )

