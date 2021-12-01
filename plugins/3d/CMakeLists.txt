add_subdirectory( idf )
add_subdirectory( vrml )

if( KICAD_USE_OCC )
    add_subdirectory( oce )
endif( KICAD_USE_OCC )
