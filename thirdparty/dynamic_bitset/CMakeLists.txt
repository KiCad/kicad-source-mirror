add_library( dynamic_bitset INTERFACE )

target_include_directories( dynamic_bitset INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )

target_sources( dynamic_bitset INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/dynamic_bitset.h
    )

target_link_libraries( dynamic_bitset INTERFACE popcnt )  # Link with libpopcnt
