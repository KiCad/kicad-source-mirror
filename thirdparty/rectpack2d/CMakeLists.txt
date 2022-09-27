add_library( rectpack2d INTERFACE )

target_include_directories( rectpack2d INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )

target_sources( rectpack2d INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/rectpack2d/best_bin_finder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/rectpack2d/empty_space_allocators.h
    ${CMAKE_CURRENT_SOURCE_DIR}/rectpack2d/empty_spaces.h
    ${CMAKE_CURRENT_SOURCE_DIR}/rectpack2d/finders_interface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/rectpack2d/insert_and_split.h
    ${CMAKE_CURRENT_SOURCE_DIR}/rectpack2d/rect_structs.h
    )
