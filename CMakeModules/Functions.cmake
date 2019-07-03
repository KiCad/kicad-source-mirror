#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
#  Copyright (C) 2010 Kicad Developers, see AUTHORS.txt for contributors.
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
#


# Function make_lexer
# is a standard way to invoke TokenList2DsnLexer.cmake.
# Extra arguments are treated as source files which depend on the generated
# files.  Some detail here on the indirection:
#  - Parallel builds all depend on the same files, and CMake will generate the same file multiple times in the same location.
# This can be problematic if the files are generated at the same time and overwrite each other.
#  - To fix this, we create a custom target (outputTarget) that the parallel builds depend on.
# AND build dependencies.  This creates the needed rebuild for appropriate source object changes.
function( make_lexer outputTarget inputFile outHeaderFile outCppFile enum )

    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${outHeaderFile}
                ${CMAKE_CURRENT_BINARY_DIR}/${outCppFile}
        COMMAND ${CMAKE_COMMAND}
            -Denum=${enum}
            -DinputFile=${CMAKE_CURRENT_SOURCE_DIR}/${inputFile}
            -DoutHeaderFile=${CMAKE_CURRENT_BINARY_DIR}/${outHeaderFile}
            -DoutCppFile=${CMAKE_CURRENT_BINARY_DIR}/${outCppFile}
            -P ${CMAKE_MODULE_PATH}/TokenList2DsnLexer.cmake
        COMMENT "TokenList2DsnLexer.cmake creating:
           ${outHeaderFile} and
           ${outCppFile} from
           ${inputFile}"
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${inputFile}
                ${CMAKE_MODULE_PATH}/TokenList2DsnLexer.cmake
        )

    target_sources( ${outputTarget} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${outCppFile} )
    target_include_directories( ${outputTarget} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} )
endfunction()


# Is a macro instead of function so there's a higher probability that the
# scope of CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA is global
macro( add_conffiles )
    if( ${ARGC} STREQUAL "0" )
        # remove the file when user passes no arguments, which he should do exactly once at top
        file( REMOVE ${CMAKE_CURRENT_BINARY_DIR}/conffiles )
    else()
        foreach( filename ${ARGV} )
            file( APPEND ${CMAKE_CURRENT_BINARY_DIR}/conffiles "${filename}\n" )
        endforeach()
        set( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA ${CMAKE_CURRENT_BINARY_DIR}/conffiles )
    endif()
endmacro( add_conffiles )
