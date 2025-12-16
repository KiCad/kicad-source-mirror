# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
# or you may search the http://www.gnu.org website for the version 2 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

option(KICAD_BUILD_FUZZ_TESTS "Build fuzz tests" OFF)

if(KICAD_BUILD_FUZZ_TESTS)
    function(add_fuzz_flags target_name)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${target_name} PRIVATE -fsanitize=fuzzer,address)
            target_link_options(${target_name} PRIVATE -fsanitize=fuzzer,address)
            target_compile_definitions(${target_name} PRIVATE KICAD_FUZZING)
        elseif(MSVC)
            target_compile_options(${target_name} PRIVATE /fsanitize=fuzzer /fsanitize=address)
            target_compile_definitions(${target_name} PRIVATE KICAD_FUZZING _DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)
        endif()
    endfunction()

    add_executable(qa_fuzz_pcb_sexpr "fuzz_pcb_sexpr.cpp" "fuzz_init.cpp")

    target_compile_definitions( qa_fuzz_pcb_sexpr
        PRIVATE PCBNEW
    )

    target_include_directories( qa_fuzz_pcb_sexpr PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/3d-viewer
        ${CMAKE_SOURCE_DIR}/common
        ${CMAKE_SOURCE_DIR}/pcbnew
        ${CMAKE_SOURCE_DIR}/pcbnew/router
        ${CMAKE_SOURCE_DIR}/pcbnew/tools
        ${CMAKE_SOURCE_DIR}/pcbnew/dialogs
        ${CMAKE_SOURCE_DIR}/polygon
        ${CMAKE_SOURCE_DIR}/common/geometry
        ${CMAKE_SOURCE_DIR}/qa/qa_utils
        ${CMAKE_SOURCE_DIR}/qa/mocks/include
    )

    # Anytime we link to the kiface_objects, we have to add a dependency on the last object
    # to ensure that the generated lexer files are finished being used before the qa runs in a
    # multi-threaded build
    add_dependencies( qa_fuzz_pcb_sexpr pcbnew )

    target_link_libraries( qa_fuzz_pcb_sexpr
        PRIVATE
            pcbnew_kiface_objects
            qa_pcbnew_utils
            3d-viewer
            connectivity
            pcbcommon
            pnsrouter
            gal
            dxflib_qcad
            tinyspline_lib
            nanosvg
            idf3
            common
            qa_utils
            markdown_lib
            scripting
            ${PCBNEW_IO_LIBRARIES}
            ${wxWidgets_LIBRARIES}
            ${GDI_PLUS_LIBRARIES}
            ${PYTHON_LIBRARIES}
            Boost::headers
            ${PCBNEW_EXTRA_LIBS}    # -lrt must follow Boost
    )

    if( WIN32 )
    # Copy dynamic lib dependency to build dir to allow running directly
    add_custom_command( TARGET qa_fuzz_pcb_sexpr POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:kicad_3dsg>" "$<TARGET_FILE_DIR:qa_fuzz_pcb_sexpr>"
        )
    endif()

    add_fuzz_flags(qa_fuzz_pcb_sexpr)

    set( PCB_KEYWORDS_FILE "${CMAKE_SOURCE_DIR}/common/pcb.keywords" )
    set( PCB_DICT_FILE "${CMAKE_SOURCE_DIR}/qa/data/fuzz/dict/pcb.dict" )

    add_custom_command(
        OUTPUT ${PCB_DICT_FILE}
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${PCB_KEYWORDS_FILE}
            -DOUTPUT_FILE=${PCB_DICT_FILE}
            -P ${CMAKE_SOURCE_DIR}/cmake/keywords_to_dict.cmake
        DEPENDS ${PCB_KEYWORDS_FILE} ${CMAKE_SOURCE_DIR}/cmake/keywords_to_dict.cmake
        COMMENT "Generating libfuzzer dictionary from pcb.keywords"
        VERBATIM
    )

    add_custom_target( qa_fuzz_dict ALL DEPENDS ${PCB_DICT_FILE} )

endif()
