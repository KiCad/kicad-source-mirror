add_library( gzip-hpp INTERFACE )

target_include_directories( gzip-hpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )

target_sources( gzip-hpp INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/decompress.hpp
    )