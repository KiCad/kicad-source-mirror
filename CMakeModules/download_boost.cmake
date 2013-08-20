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

set( BOOST_RELEASE 1.53.0 )
set( BOOST_MD5 a00d22605d5dbcfb4c9936a9b35bc4c2 )   # re-calc this on every RELEASE change

# The boost headers [and static libs if built] go here, at the top of KiCad
# source tree in boost_root.
set( BOOST_ROOT "${PROJECT_SOURCE_DIR}/boost_root" )


if( BUILD_GITHUB_PLUGIN )
    # Space separated list which indicates the subset of boost libraries to compile.
    set( BOOST_LIBS_BUILT
        filesystem
        system
        regex
        program_options
        date_time
        thread
        exception
        unit_test_framework
        )
endif()

#-----</configure>---------------------------------------------------------------



string( REGEX REPLACE "\\." "_" BOOST_VERS "${BOOST_RELEASE}" )
set( PREFIX ${DOWNLOAD_DIR}/boost_${BOOST_VERS} )

# <SOURCE_DIR> = ${PREFIX}/src/boost
# There is a Bazaar 'boost scratch repo' in <SOURCE_DIR>/boost and after committing pristine
# download, the patch is applied.  This lets you regenerate a new patch at any time
# easily, simply by editing the working tree in <SOURCE_DIR> and doing "bzr diff" in there.

# path to the boost headers in the repo.
# repo = "${headers_src}/../.bzr" = "<SOURCE_DIR>/.bzr"
set( headers_src "${PREFIX}/src/boost/boost" )


# don't look at this:
function( set_boost_lib_names libs )
    foreach( lib IN LISTS ${libs} )
        string( TOUPPER ${lib} LIB )
        message( STATUS "LIB:${LIB} lib:${lib}" )
        set( Boost_${LIB}_LIBRARY, "${BOOST_ROOT}/lib/libboost_${lib}.a" PARENT_SCOPE )
        set( Boost_LIBRARIES ${Boost_LIBRARIES} ${Boost_${LIB}_LIBRARY} PARENT_SCOPE )
    endforeach()
endfunction()


if( BUILD_GITHUB_PLUGIN )

    message( FATAL_ERROR
        "BUILD_GITHUB_PLUGIN not functional. With this commit we get merely a greenfield for better boost usage and building."
        )

    # (BTW "test" yields "unit_test_framework" when passed to bootstrap.{sh,bat} ).
    message( STATUS "BOOST_LIBS_BUILT:${BOOST_LIBS_BUILT}" )
    string( REPLACE "unit_test_framework" "test" libs_csv "${BOOST_LIBS_BUILT}" )
    message( STATUS "REPLACE libs_csv:${libs_csv}" )

    string( REGEX REPLACE "\\;" "," libs_csv "${libs_csv}" )
    message( STATUS "libs_csv:${libs_csv}" )

    if( MINGW )
        set( bootstrap "bootstart.bat mingw" )
    else()
        set( bootstrap bootstrap.sh )
    endif()

    ExternalProject_Add( boost
        PREFIX          "${PREFIX}"
        DOWNLOAD_DIR    "${DOWNLOAD_DIR}"
        URL             http://downloads.sourceforge.net/project/boost/boost/${BOOST_RELEASE}/boost_${BOOST_VERS}.tar.bz2
        URL_MD5         ${BOOST_MD5}

        # The patch command executes with the working directory set to <SOURCE_DIR>
        PATCH_COMMAND   bzr patch -p0 "${PROJECT_SOURCE_DIR}/patches/boost.patch"

        # [Mis-]use this step to erase all the boost headers and libraries before
        # replacing them below.
        UPDATE_COMMAND  ${CMAKE_COMMAND} -E remove_directory "${BOOST_ROOT}"

        BINARY_DIR      "${PREFIX}/src/boost/"
        CONFIGURE_COMMAND ${bootstrap}
                        --with-libraries=${libs_csv}

        BUILD_COMMAND   b2
                        variant=release
                        threading=multi
                        toolset=gcc
                        link=static
                        --prefix=${BOOST_ROOT}
                        install

        INSTALL_COMMAND ""
        )


else( BUILD_GITHUB_PLUGIN )


    ExternalProject_Add( boost
        PREFIX          "${PREFIX}"
        DOWNLOAD_DIR    "${DOWNLOAD_DIR}"
        URL             http://downloads.sourceforge.net/project/boost/boost/${BOOST_RELEASE}/boost_${BOOST_VERS}.tar.bz2
        URL_MD5         ${BOOST_MD5}

        # The patch command executes with the working directory set to <SOURCE_DIR>
        PATCH_COMMAND   bzr patch -p0 "${PROJECT_SOURCE_DIR}/patches/boost.patch"

        # Dick 18-Aug-2013:
        # [mis-]use this UPDATE_COMMAND opportunity to remove the old place of boost headers.
        # Can eventually remove this step after headers are moved from <kicad_src>/include/boost
        # to <kicad_src>/boost_root/include/boost over the next several months.
        UPDATE_COMMAND  ${CMAKE_COMMAND} -E remove_directory "${PROJECT_SOURCE_DIR}/include/boost"

        CONFIGURE_COMMAND ""

        # remove then re-copy into the include/boost directory during next two steps:
        BUILD_COMMAND   ${CMAKE_COMMAND} -E remove_directory ${BOOST_ROOT}
        INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory "${headers_src}" "${BOOST_ROOT}/include/boost"
        )

    # Until my find_package() support is done for my boost.
    set( Boost_INCLUDE_DIR  "${BOOST_ROOT}/include" CACHE FILEPATH "Boost library directory" )


endif( BUILD_GITHUB_PLUGIN )


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

