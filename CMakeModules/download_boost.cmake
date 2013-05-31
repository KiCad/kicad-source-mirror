
# Download and patch boost headers to a particular version.
# Assumes include( ExternalProject ) was done inline previous to this file.

set( BOOST_RELEASE 1.53.0 )
string( REGEX REPLACE "\\." "_" BOOST_VERS "${BOOST_RELEASE}" )

set( BOOST_MD5 a00d22605d5dbcfb4c9936a9b35bc4c2 ) # re-calc this on every RELEASE change
set( PREFIX ${DOWNLOAD_DIR}/boost_${BOOST_VERS} )


ExternalProject_Add(
    boost
    PREFIX          ${PREFIX}
    DOWNLOAD_DIR    ${DOWNLOAD_DIR}
    URL http://downloads.sourceforge.net/project/boost/boost/${BOOST_RELEASE}/boost_${BOOST_VERS}.tar.bz2
    URL_MD5 ${BOOST_MD5}

    #UPDATE_COMMAND

    # The patch command executes with the working directory set to <SOURCE_DIR>
    PATCH_COMMAND patch -p0 < ${PROJECT_SOURCE_DIR}/patches/boost.patch

    CONFIGURE_COMMAND ""

    # remove then re-copy into the include/boost directory during next two steps:
    BUILD_COMMAND ${CMAKE_COMMAND} -E remove_directory ${PROJECT_SOURCE_DIR}/include/boost
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/boost ${PROJECT_SOURCE_DIR}/include/boost
    )


# <SOURCE_DIR> = ${PREFIX}/src/
# Add extra steps, so that we can easily regenerate any boost patch needed for the above.
# There is a Bazaar 'boost scratch repo' in <SOURCE_DIR> and after committing pristine
# download, the patch is applied.  This lets you regenerate a new patch at any time
# easily, simply by editing the copy in <SOURCE_DIR> and doing "bzr diff" in there.


ExternalProject_Add_Step( boost bzr_commit_boost
    COMMAND bzr ci -q -m pristine <SOURCE_DIR>
    COMMENT "committing boost files to 'boost scratch repo'"
    DEPENDERS patch
    )


ExternalProject_Add_Step( boost bzr_add_boost
    COMMAND bzr add -q <SOURCE_DIR>
    COMMENT "adding boost files to 'boost scratch repo'"
    DEPENDERS bzr_commit_boost
    )


ExternalProject_Add_Step( boost bzr_init_boost
    COMMAND bzr init -q <SOURCE_DIR>
    COMMENT "creating 'boost scratch repo' specifically for boost to track boost patches"
    DEPENDERS bzr_commit_boost
    DEPENDEES download
    )

