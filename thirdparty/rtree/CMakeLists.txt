add_library( rtree INTERFACE )

target_include_directories( rtree INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )

target_sources( rtree INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry/rtree.h
    )
