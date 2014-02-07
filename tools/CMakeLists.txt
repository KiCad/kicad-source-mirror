
set( MAKE_LINK_MAPS true )

if( 0 )

    project(kicad-tools)

    cmake_minimum_required( VERSION 2.8 FATAL_ERROR )

    set( PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ )

    # message( "PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}" )

    # Path to local CMake modules.
    set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules )

    include( CheckFindPackageResult )

    ##########################
    # Find wxWidgets library #
    ##########################
    # Here you can define what libraries of wxWidgets you need for your
    # application. You can figure out what libraries you need here;
    # http://www.wxwidgets.org/manuals/2.8/wx_librarieslist.html

    # On Apple only wxwidgets 2.9 or higher doesn't need to find aui part of base
    if(APPLE)
        find_package(wxWidgets COMPONENTS gl adv html core net base xml QUIET)
    else(APPLE)
        find_package(wxWidgets COMPONENTS gl aui adv html core net base xml QUIET)
    endif(APPLE)
    check_find_package_result(wxWidgets_FOUND "wxWidgets")


    # Include wxWidgets macros.
    include(${wxWidgets_USE_FILE})

    # make config.h
    include( PerformFeatureChecks )
    perform_feature_checks()


endif()

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/pcbnew
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    )


add_executable( container_test
    EXCLUDE_FROM_ALL
    container_test.cpp
    )
target_link_libraries( container_test
    common
    polygon
    bitmaps
    ${wxWidgets_LIBRARIES}
    )

add_executable( test-nm-biu-to-ascii-mm-round-tripping
    EXCLUDE_FROM_ALL
    test-nm-biu-to-ascii-mm-round-tripping.cpp
    )

add_executable( property_tree
    EXCLUDE_FROM_ALL
    property_tree.cpp
    ../common/richio.cpp
    ../common/dsnlexer.cpp
    ../common/ptree.cpp
    )
target_link_libraries( property_tree
    ${wxWidgets_LIBRARIES}
    )


#-----<KIWAY Low Level DSO Testing>---------------------------------------------

# The small launcher, it sets up wxWidgets library and loads a MODULE by the same name
# but with extension ${KIFACE_SUFFIX}.

set( PAIR_BASE kiway_test )

add_executable( ${PAIR_BASE} WIN32 MACOSX_BUNDLE
    EXCLUDE_FROM_ALL
    ../common/single_top.cpp
    )
target_link_libraries( ${PAIR_BASE}
    common
    ${wxWidgets_LIBRARIES}
    )
if( APPLE )
    set_target_properties( ${PAIR_BASE} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        )
endif()


# make a KIFACE top level DLL/DSO
add_library( ${PAIR_BASE}_kiface MODULE
    EXCLUDE_FROM_ALL
    kiface_test.cpp
    )
target_link_libraries( ${PAIR_BASE}_kiface
    common
    ${wxWidgets_LIBRARIES}
    )
set_target_properties( ${PAIR_BASE}_kiface PROPERTIES
    OUTPUT_NAME     ${PAIR_BASE}
    PREFIX          ${KIFACE_PREFIX}
    SUFFIX          ${KIFACE_SUFFIX}
    )

# if you build ${PAIR_BASE}, then also build ${PAIR_BASE}_kiface if out of date.
add_dependencies( ${PAIR_BASE} ${PAIR_BASE}_kiface )

if( MAKE_LINK_MAPS )

    # generate a link maps with cross reference

    set_target_properties( ${PAIR_BASE}_kiface PROPERTIES
        LINK_FLAGS "${TO_LINKER},-cref ${TO_LINKER},-Map=${KIFACE_PREFIX}${PAIR_BASE}${KIFACE_SUFFIX}.map"
        )

    set_target_properties( ${PAIR_BASE} PROPERTIES
        LINK_FLAGS "${TO_LINKER},-cref ${TO_LINKER},-Map=${PAIR_BASE}.map"
        )

endif()
