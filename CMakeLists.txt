cmake_minimum_required( VERSION 2.8.4 FATAL_ERROR )

if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
    message( FATAL_ERROR
        "In-source builds not allowed. Please make a new directory (called"
        " a build directory) and run CMake from there, points here."
        " You may need to remove CMakeCache.txt." )
endif()

project( kicad-i18n NONE )

find_package( Gettext REQUIRED )

option( KICAD_I18N_UNIX_STRICT_PATH
    "Use standard Unix locale lookup path (default OFF)."
    OFF
    )

if( UNIX AND KICAD_I18N_UNIX_STRICT_PATH )
    set( KICAD_I18N_PATH ${CMAKE_INSTALL_PREFIX}/share/locale
        CACHE PATH "Location of KiCad i18n files." )
else()
    # Default Unix inconvenient locale lookup path
    set( KICAD_I18N_PATH share/kicad/internat
        CACHE PATH "Location of KiCad i18n files." )
endif()

file(STRINGS LINGUAS LANG_ARRAY REGEX "^[^#].*")

foreach( LANG ${LANG_ARRAY} )
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${LANG})

    execute_process( COMMAND ${GETTEXT_MSGFMT_EXECUTABLE}
                     ${CMAKE_SOURCE_DIR}/${LANG}/kicad.po
                     -o ${CMAKE_BINARY_DIR}/${LANG}/kicad.mo )

    if( UNIX AND KICAD_I18N_UNIX_STRICT_PATH )
        install( FILES ${CMAKE_BINARY_DIR}/${LANG}/kicad.mo
                DESTINATION ${KICAD_I18N_PATH}/${LANG}/LC_MESSAGES
                COMPONENT resources )
    else()
        install( FILES ${CMAKE_BINARY_DIR}/${LANG}/kicad.mo
                DESTINATION ${KICAD_I18N_PATH}/${LANG}
                COMPONENT resources )
    endif()
endforeach()

