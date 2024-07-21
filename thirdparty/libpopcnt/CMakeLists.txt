add_library( popcnt INTERFACE )

target_include_directories( popcnt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )

target_sources( popcnt INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/libpopcnt.h
    )
