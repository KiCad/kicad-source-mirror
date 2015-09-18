cmake_minimum_required( VERSION 2.8.4 FATAL_ERROR )

project( kicad-i18n NONE )

find_package( Gettext REQUIRED )

if( UNIX AND NOT APPLE )
set( KICAD_I18N ${CMAKE_INSTALL_PREFIX}/share/locale
    CACHE PATH "Location of KiCad i18n files." )
else()
set( KICAD_I18N ${CMAKE_INSTALL_PREFIX}/share/kicad/internat
    CACHE PATH "Location of KiCad i18n files." )
endif()

set( LANG_ARRAY ca cs de es fi fr hu it ja ko nl pl pt ru sl sv zh_CN )
foreach( LANG ${LANG_ARRAY} )
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${LANG})

    execute_process( COMMAND ${GETTEXT_MSGFMT_EXECUTABLE}
                     ${CMAKE_SOURCE_DIR}/${LANG}/kicad.po
                     -o ${CMAKE_BINARY_DIR}/${LANG}/kicad.mo )

    if( UNIX AND NOT APPLE )
        install( FILES ${CMAKE_BINARY_DIR}/${LANG}/kicad.mo
                DESTINATION ${KICAD_I18N}/${LANG}/LC_MESSAGES
                COMPONENT resources )
    else()
        install( FILES ${CMAKE_BINARY_DIR}/${LANG}/kicad.mo
                DESTINATION ${KICAD_I18N}/${LANG}
                COMPONENT resources )
    endif()
endforeach()

