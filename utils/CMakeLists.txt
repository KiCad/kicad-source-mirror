# .cpp files are compiled with extra ${WSHADOW_FLAGS}
if( COMPILER_SUPPORTS_WSHADOW )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WSHADOW_FLAGS}")
endif()

add_subdirectory( idftools )

if( KICAD_USE_OCE )
    add_subdirectory( kicad2step )
endif( KICAD_USE_OCE )
