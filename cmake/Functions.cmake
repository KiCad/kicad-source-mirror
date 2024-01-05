#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
#  Copyright (C) 2010-2020 Kicad Developers, see AUTHORS.txt for contributors.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, you may find one here:
#  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
#  or you may search the http://www.gnu.org website for the version 2 license,
#  or you may write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA


# make_lexer_export
# This function performs the same job as make_lexer but with two additional parameters to specify
# the export macro
function( make_lexer_export outputTarget inputFile outHeaderFile outCppFile enum exportMacro exportMacroInclude )
    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${outHeaderFile}
                ${CMAKE_CURRENT_BINARY_DIR}/${outCppFile}
        COMMAND ${CMAKE_COMMAND}
            -Denum=${enum}
            -DinputFile=${CMAKE_CURRENT_SOURCE_DIR}/${inputFile}
            -DoutHeaderFile=${CMAKE_CURRENT_BINARY_DIR}/${outHeaderFile}
            -DoutCppFile=${CMAKE_CURRENT_BINARY_DIR}/${outCppFile}
            -DexportMacro=${exportMacro}
            -DexportMacroInclude=${exportMacroInclude}
            -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/TokenList2DsnLexer.cmake
        COMMENT "TokenList2DsnLexer.cmake creating:
           ${outHeaderFile} and
           ${outCppFile} from
           ${inputFile}"
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${inputFile}
                ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/TokenList2DsnLexer.cmake
        )

    target_sources( ${outputTarget} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${outCppFile} )
    target_include_directories( ${outputTarget} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} )
endfunction()


# Function make_lexer
# is a standard way to invoke TokenList2DsnLexer.cmake.
# Extra arguments are treated as source files which depend on the generated
# files.  Some detail here on the indirection:
#  - Parallel builds all depend on the same files, and CMake will generate the same file multiple times in the same location.
# This can be problematic if the files are generated at the same time and overwrite each other.
#  - To fix this, we create a custom target (outputTarget) that the parallel builds depend on.
# AND build dependencies.  This creates the needed rebuild for appropriate source object changes.
function( make_lexer outputTarget inputFile outHeaderFile outCppFile enum )
    make_lexer_export( ${outputTarget} ${inputFile} ${outHeaderFile} ${outCppFile} ${enum} "" "" )
endfunction()


# Function generate_lemon_grammar
#
# This is a function to create a custom command to generate a parser grammar using lemon.
#
# Arguments:
#  - TGT is the target to add the consuming file to
#  - GRAMMAR_DIR is the path relative to CMAKE_CURRENT_BINARY_DIR for the directory where the files will be generated into
#  - CONSUMING_FILE is the file relative to CMAKE_CURRENT_SOURCE_DIR that will include the grammar.c/h file
#  - GRAMMAR_FILE is the file relative to CMAKE_CURRENT_SOURCE_DIR of the grammar file to use.
function( generate_lemon_grammar TGT GRAMMAR_DIR CONSUMING_FILE GRAMMAR_FILE )
    # Get the name without extension
    get_filename_component( GRAMMAR_BASE ${GRAMMAR_FILE} NAME_WE )

    file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${GRAMMAR_DIR} )

    set( LEMON_EXE $<TARGET_FILE:lemon> CACHE FILEPATH "Path to Lemon Executable" )

    get_property( LEMON_TEMPLATE
        TARGET lemon
        PROPERTY lemon_template
        )

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${GRAMMAR_DIR}/${GRAMMAR_BASE}.c
               ${CMAKE_CURRENT_BINARY_DIR}/${GRAMMAR_DIR}/${GRAMMAR_BASE}.h
        COMMAND ${CMAKE_COMMAND}
            -DLEMON_EXE=${LEMON_EXE}
            -DLEMON_TEMPLATE=${LEMON_TEMPLATE}
            -DGRAMMAR_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${GRAMMAR_FILE}
            -DGRAMMAR_DIR=${CMAKE_CURRENT_BINARY_DIR}/${GRAMMAR_DIR}
            -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/LemonParserGenerator.cmake
        COMMENT "Running Lemon on ${GRAMMAR_FILE} to generate ${GRAMMAR_DIR}/${GRAMMAR_BASE}.c"
        DEPENDS lemon
                ${CMAKE_CURRENT_SOURCE_DIR}/${GRAMMAR_FILE}
                ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/LemonParserGenerator.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${GRAMMAR_DIR}
    )

    if(MSVC)
        # lemon has a habit of generating empty switch cases which we can ignore
        set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/${CONSUMING_FILE} PROPERTIES COMPILE_FLAGS /wd4065)
    endif()

    # Mark the consuming file with a direct dependency on the generated grammar so that
    # it isn't compiled until the grammar is generated
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/${CONSUMING_FILE}
        PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${GRAMMAR_DIR}/${GRAMMAR_BASE}.c
    )

    target_sources( ${TGT} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${CONSUMING_FILE} )
    target_include_directories( ${TGT} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} )
endfunction()


# Function translate_language
#
# This is a function to add the targets and install step for translating a language
#
# Arguments:
#  - LANG is the code for the language (which must be the same as the directory name)
#  - OUT_FILE is the file (including directory) to save the translations to
function( translate_language LANG OUT_FILE)
    # Make the output directory (if it doesn't already exist)
    get_filename_component( OUT_DIR ${OUT_FILE} DIRECTORY )

    file( MAKE_DIRECTORY ${OUT_DIR} )

    add_custom_command(
        OUTPUT ${OUT_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/pofiles/${LANG}.po
        COMMAND ${GETTEXT_MSGFMT_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/pofiles/${LANG}.po
                -o ${OUT_FILE}
        COMMENT "Building translation library for ${LANG}"
        )

    if( UNIX AND KICAD_I18N_UNIX_STRICT_PATH )
        install( FILES ${OUT_FILE}
                DESTINATION ${KICAD_I18N_PATH}/${LANG}/LC_MESSAGES
                COMPONENT resources )
    else()
        install( FILES ${OUT_FILE}
                DESTINATION ${KICAD_I18N_PATH}/${LANG}
                COMPONENT resources )
    endif()
endfunction()


# Function linux_metadata_translation
#
# This is a macro to handle the translation of the linux metadata files using
# the existing .po files.
#
# Arguments:
#  - SRC_FILE is the file to use as the translation source
#  - OUT_FILE is the file (including directory) to save the translations to
#  - PO_DIR is the directory containing the raw po files
macro( linux_metadata_translation SRC_FILE OUT_FILE PO_DIR )
    get_filename_component( OUT_DIR ${OUT_FILE} DIRECTORY )
    get_filename_component( OUT_FNAME ${OUT_FILE} NAME )

    file( MAKE_DIRECTORY ${OUT_DIR} )


    # Find all the po files to setup the dependency chain
    file( STRINGS ${PO_DIR}/LINGUAS LANG_ARRAY REGEX "^[^#].*" )

    set( LANG_FILES "" )

    foreach( LANG ${LANG_ARRAY} )
        # Keep a list of the language files that are created to add to the target
        list( APPEND LANG_FILES "${PO_DIR}/${LANG}.po" )
    endforeach()


    # Add the command to translate the file
    if( KICAD_BUILD_I18N )
        add_custom_command(
            OUTPUT ${OUT_FILE}
            DEPENDS ${SRC_FILE}
                    ${LANG_FILES}
                    ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/TranslatePlatformMetadata_linux.cmake
            COMMAND ${CMAKE_COMMAND}
                    -DMSGFMT_EXE="${GETTEXT_MSGFMT_EXECUTABLE}"
                    -DPO_DIR="${PO_DIR}"
                    -DSRC_FILE="${SRC_FILE}"
                    -DDEST_FILE="${OUT_FILE}"
                    -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/TranslatePlatformMetadata_linux.cmake
            COMMENT "Translating file ${OUT_FNAME}"
            )
    else()
        add_custom_command(
            OUTPUT ${OUT_FILE}
            DEPENDS ${SRC_FILE}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_FILE}" "${OUT_FILE}"
            COMMENT "Copying file ${OUT_FNAME}"
            )
    endif()
endmacro()
