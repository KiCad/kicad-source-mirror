# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

add_subdirectory( idftools )

if( KICAD_USE_OCC )
    add_subdirectory( kicad2step )
endif( KICAD_USE_OCC )
