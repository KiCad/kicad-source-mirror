
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

add_executable( utf8_tests
    EXCLUDE_FROM_ALL
    utf8_tests.cpp
    )
target_link_libraries( utf8_tests
    common
    ${wxWidgets_LIBRARIES}
    )

add_subdirectory( newstroke )