# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()


add_library( core INTERFACE
    # Just a header library right now
)

target_link_libraries( core INTERFACE
    ${wxWidgets_LIBRARIES}
)

target_include_directories( core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
