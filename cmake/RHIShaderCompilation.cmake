# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

# Compiles the RHI's Vulkan GLSL shaders to SPIR-V, and on Apple to MSL, and embeds them in generated C++ headers.

# vcpkg installs host tools under the host triplet, which its toolchain leaves off the program path when
# cross-compiling
if( VCPKG_INSTALLED_DIR )
    file( GLOB RHI_VCPKG_TOOL_DIRS LIST_DIRECTORIES true "${VCPKG_INSTALLED_DIR}/*/tools/*" )
endif()

set( RHI_SHADER_TOOLS GLSLANG_VALIDATOR:glslangValidator SPIRV_OPT:spirv-opt SPIRV_VAL:spirv-val )

# Only the Metal backend reads MSL
if( APPLE )
    list( APPEND RHI_SHADER_TOOLS SPIRV_CROSS:spirv-cross )
endif()

# spirv-val is optional, the others are needed to build the shaders the RHI includes
foreach( tool ${RHI_SHADER_TOOLS} )
    string( REPLACE ":" ";" tool "${tool}" )
    list( GET tool 0 var )
    list( GET tool 1 program )

    find_program( ${var} ${program} HINTS ENV VULKAN_SDK ${RHI_VCPKG_TOOL_DIRS} PATH_SUFFIXES bin )

    if( NOT ${var} AND NOT var STREQUAL "SPIRV_VAL" )
        message( FATAL_ERROR "RHI shader tool ${program} not found. Install the Vulkan SDK or set VULKAN_SDK." )
    endif()

    message( STATUS "RHI shader tool ${program}: ${${var}}" )
endforeach()

# kicad_add_shaders( TARGET target SHADERS shader1.vert shader2.frag ... )
#
# Generates rhi_shaders/<name>_<stage>.h per shader in the binary dir plus rhi_shaders.h including them all,
# and adds that directory to the target's include path.
function( kicad_add_shaders )
    cmake_parse_arguments( SHADER "" "TARGET" "SHADERS" ${ARGN} )

    if( NOT SHADER_TARGET )
        message( FATAL_ERROR "kicad_add_shaders: TARGET is required" )
    endif()

    if( NOT SHADER_SHADERS )
        message( FATAL_ERROR "kicad_add_shaders: SHADERS list is required" )
    endif()

    set( SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/rhi_shaders" )
    file( MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}" )

    set( ALL_GENERATED_HEADERS "" )

    # Editing a shared .glsl include must rebuild every shader, since any of them may include it.
    file( GLOB SHADER_INCLUDES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.glsl" )

    set( SHADER_STAGES vert frag comp )

    foreach( SHADER_SRC ${SHADER_SHADERS} )
        get_filename_component( SHADER_NAME "${SHADER_SRC}" NAME_WE )
        get_filename_component( SHADER_STAGE "${SHADER_SRC}" LAST_EXT )
        get_filename_component( SHADER_FULL "${SHADER_SRC}" ABSOLUTE )
        string( SUBSTRING "${SHADER_STAGE}" 1 -1 SHADER_STAGE )

        if( NOT SHADER_STAGE IN_LIST SHADER_STAGES )
            message( FATAL_ERROR "kicad_add_shaders: unknown shader stage '${SHADER_STAGE}'" )
        endif()

        set( SPV_FILE    "${SHADER_OUTPUT_DIR}/${SHADER_NAME}_${SHADER_STAGE}.spv" )
        set( SPV_OPT     "${SHADER_OUTPUT_DIR}/${SHADER_NAME}_${SHADER_STAGE}_opt.spv" )
        set( MSL_FILE    "${SHADER_OUTPUT_DIR}/${SHADER_NAME}_${SHADER_STAGE}.msl" )
        set( HEADER_FILE "${SHADER_OUTPUT_DIR}/${SHADER_NAME}_${SHADER_STAGE}.h" )

        add_custom_command(
            OUTPUT "${SPV_FILE}"
            COMMAND "${GLSLANG_VALIDATOR}" -V -o "${SPV_FILE}" "${SHADER_FULL}"
            DEPENDS "${SHADER_FULL}" ${SHADER_INCLUDES}
            COMMENT "Compiling ${SHADER_SRC} to SPIR-V"
            VERBATIM
        )

        add_custom_command(
            OUTPUT "${SPV_OPT}"
            COMMAND "${SPIRV_OPT}" -O "${SPV_FILE}" -o "${SPV_OPT}"
            DEPENDS "${SPV_FILE}"
            COMMENT "Optimizing ${SHADER_NAME}_${SHADER_STAGE}.spv"
            VERBATIM
        )

        if( APPLE )
            # Keep each binding number as its Metal index so the device can derive every index from the layout.
            add_custom_command(
                OUTPUT "${MSL_FILE}"
                COMMAND "${SPIRV_CROSS}" --msl --msl-version 20000 --msl-decoration-binding
                        --output "${MSL_FILE}" "${SPV_OPT}"
                DEPENDS "${SPV_OPT}"
                COMMENT "Cross-compiling ${SHADER_NAME} to MSL"
                VERBATIM
            )
        else()
            set( MSL_FILE "" )
        endif()

        add_custom_command(
            OUTPUT "${HEADER_FILE}"
            COMMAND ${CMAKE_COMMAND}
                -DSHADER_NAME=${SHADER_NAME}
                -DSHADER_STAGE=${SHADER_STAGE}
                -DSPV_FILE=${SPV_OPT}
                -DMSL_FILE=${MSL_FILE}
                -DHEADER_FILE=${HEADER_FILE}
                -P ${CMAKE_SOURCE_DIR}/cmake/BuildSteps/GenerateShaderHeader.cmake
            DEPENDS "${SPV_OPT}" ${MSL_FILE}
                    "${CMAKE_SOURCE_DIR}/cmake/BuildSteps/GenerateShaderHeader.cmake"
            COMMENT "Generating header for ${SHADER_NAME}_${SHADER_STAGE}"
        )

        list( APPEND ALL_GENERATED_HEADERS "${HEADER_FILE}" )
    endforeach()

    # One header that includes every generated one
    set( UMBRELLA "// Auto-generated by kicad_add_shaders -- do not edit\n#pragma once\n\n" )

    foreach( HEADER_FILE ${ALL_GENERATED_HEADERS} )
        get_filename_component( HEADER_NAME "${HEADER_FILE}" NAME )
        string( APPEND UMBRELLA "#include <${HEADER_NAME}>\n" )
    endforeach()

    file( CONFIGURE OUTPUT "${SHADER_OUTPUT_DIR}/rhi_shaders.h" CONTENT "${UMBRELLA}" )

    add_custom_target( ${SHADER_TARGET}_shaders DEPENDS ${ALL_GENERATED_HEADERS} )
    add_dependencies( ${SHADER_TARGET} ${SHADER_TARGET}_shaders )
    target_include_directories( ${SHADER_TARGET} PRIVATE "${SHADER_OUTPUT_DIR}" )
endfunction()
