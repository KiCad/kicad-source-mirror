add_library( nanoflann INTERFACE )

target_include_directories( nanoflann INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )

target_sources( nanoflann INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/nanoflann.hpp
    )