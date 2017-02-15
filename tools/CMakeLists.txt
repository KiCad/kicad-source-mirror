
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
    ../common/exceptions.cpp
    ../common/dsnlexer.cpp
    ../common/ptree.cpp
    )
target_link_libraries( property_tree
    ${wxWidgets_LIBRARIES}
    )

add_subdirectory( io_benchmark )
