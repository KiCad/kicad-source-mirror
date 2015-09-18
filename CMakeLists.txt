cmake_minimum_required( VERSION 3.0 )

project( kicad-i18n NONE )

find_package( Gettext REQUIRED )

set( LANG_ARRAY ca cs de es fi fr hu it ja ko nl pl pt ru sl sv zh_CN )
foreach( LANG ${LANG_ARRAY} )
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${LANG})

    execute_process( COMMAND ${GETTEXT_MSGFMT_EXECUTABLE}
                     ${CMAKE_SOURCE_DIR}/${LANG}/kicad.po
                     -o ${CMAKE_BINARY_DIR}/${LANG}/kicad.mo )

    install( FILES ${CMAKE_BINARY_DIR}/${LANG}/kicad.mo
             DESTINATION share/kicad/internat
             COMPONENT resources )
endforeach()

