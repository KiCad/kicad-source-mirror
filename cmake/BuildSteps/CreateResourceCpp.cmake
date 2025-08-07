#
# Used as a standalone cmake script to process a given file into binary arrays
# at compile-time
#
# Include CreateResourceCppFunction.cmake to get access to the add_compiled_resource
# function
#
# This file is structured to be invoked from add_custom_command in order
# to have GENERATED marked output files that can be rebuilt on change.
#
# Required Arguments:
# SOURCE_FILE - Path to source shader file
# OUT_CPP_DIR - Destination path for cpp file
# OUT_HEADER_DIR - Destination path for header file
# OUT_CPP_FILENAME - cpp filename
# OUT_HEADER_FILENAME - header filename
# OUT_VAR_NAME - Name of variable containing shader to be created
#
# Parts taken from https://github.com/sivachandran/cmake-bin2h
# Copyright 2020 Sivachandran Paramasivam
#

# Function to wrap a given string into multiple lines at the given column position.
# Parameters:
#   VARIABLE    - The name of the CMake variable holding the string.
#   AT_COLUMN   - The column position at which string will be wrapped.
function(WRAP_STRING)
    set(oneValueArgs VARIABLE AT_COLUMN)
    cmake_parse_arguments(WRAP_STRING "${options}" "${oneValueArgs}" "" ${ARGN})

    string(LENGTH ${${WRAP_STRING_VARIABLE}} stringLength)
    math(EXPR offset "0")

    while(stringLength GREATER 0)

        if(stringLength GREATER ${WRAP_STRING_AT_COLUMN})
            math(EXPR length "${WRAP_STRING_AT_COLUMN}")
        else()
            math(EXPR length "${stringLength}")
        endif()

        string(SUBSTRING ${${WRAP_STRING_VARIABLE}} ${offset} ${length} line)
        set(lines "${lines}\n${line}")

        math(EXPR stringLength "${stringLength} - ${length}")
        math(EXPR offset "${offset} + ${length}")
    endwhile()

    set(${WRAP_STRING_VARIABLE} "${lines}" PARENT_SCOPE)
endfunction()


file( READ ${SOURCE_FILE} _SOURCE_BINARY HEX )
string(LENGTH ${_SOURCE_BINARY} _SOURCE_BINARY_LENGTH)

set(SOURCE_BINARY "${_SOURCE_BINARY}00") # null terminate for the sake of it

wrap_string(VARIABLE _SOURCE_BINARY AT_COLUMN 32)
math(EXPR _ARRAY_SIZE "${_SOURCE_BINARY_LENGTH} / 2")


# adds '0x' prefix and comma suffix before and after every byte respectively
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " _ARRAY_VALUES ${_SOURCE_BINARY})
# removes trailing comma
string(REGEX REPLACE ", $" "" _ARRAY_VALUES ${_ARRAY_VALUES})

set( outCppTextByteArray "unsigned char ${OUT_VAR_NAME}_bytes[] = { ${_ARRAY_VALUES} };")
set( outCppTextByteSize "size_t ${OUT_VAR_NAME}_size = ${_ARRAY_SIZE};")

set( outCppText
"
#include <stddef.h>
#include <${OUT_HEADER_FILENAME}>

namespace RESOURCE {
${outCppTextByteArray}
${outCppTextByteSize}
}
" )

file(
    WRITE ${OUT_CPP_DIR}/${OUT_CPP_FILENAME}
    "${outCppText}"
)


set( outHeaderText
"namespace RESOURCE {
        extern unsigned char ${OUT_VAR_NAME}_bytes[];
        extern size_t ${OUT_VAR_NAME}_size;
}"
)

file(
    WRITE ${OUT_HEADER_DIR}/${OUT_HEADER_FILENAME}
    "${outHeaderText}"
)

message(STATUS "Resource ${SOURCE_FILE} converted to ${OUT_CPP_DIR}/${OUT_CPP_FILENAME}")