add_subdirectory( idf )
add_subdirectory( vrml )

if( KICAD_USE_OCE )
    add_subdirectory( oce )
endif( KICAD_USE_OCE )
