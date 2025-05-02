add_library( rapidcsv INTERFACE )

target_include_directories( rapidcsv INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )

target_sources( rapidcsv INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/rapidcsv/rapidcsv.h
)
