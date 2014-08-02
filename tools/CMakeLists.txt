
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
    ${BOOST_INCLUDE}
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

