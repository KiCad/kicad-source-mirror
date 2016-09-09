add_subdirectory( idftools )

if( KICAD_USE_OCE )
    add_subdirectory( kicad2step )
endif( KICAD_USE_OCE )
