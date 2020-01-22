
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/pcbnew
    ${BOOST_INCLUDE}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    )

add_executable( test-nm-biu-to-ascii-mm-round-tripping
    EXCLUDE_FROM_ALL
    test-nm-biu-to-ascii-mm-round-tripping.cpp
    )

add_executable( property_tree
    EXCLUDE_FROM_ALL
    property_tree.cpp
    ${CMAKE_SOURCE_DIR}/common/richio.cpp
    ${CMAKE_SOURCE_DIR}/common/exceptions.cpp
    ${CMAKE_SOURCE_DIR}/common/dsnlexer.cpp
    ${CMAKE_SOURCE_DIR}/common/ptree.cpp
    )
target_link_libraries( property_tree
    ${wxWidgets_LIBRARIES}
    )


add_executable( utf8_tests
    EXCLUDE_FROM_ALL
    utf8_tests.cpp
    )
target_link_libraries( utf8_tests
    common
    ${wxWidgets_LIBRARIES}
    )
