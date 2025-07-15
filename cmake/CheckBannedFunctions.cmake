# CheckBannedFunctions.cmake
# Usage: include(CheckBannedFunctions.cmake)

# This script scans for banned functions in io directories and fails the build if found.

# Set CMP0007 to NEW to ensure list variables are not stringified
cmake_policy(SET CMP0007 NEW)

# Usage: check_banned_functions() after setting BANNED_DIRS
macro(check_banned_functions)
    set(BANNED_FUNCTIONS "strtod" "strtof" "atof")
    set(BANNED_SCAN_FILES)

    # Ensure BANNED_DIRS is set and not empty
    if(NOT DEFINED BANNED_DIRS OR "${BANNED_DIRS}" STREQUAL "")
        message(FATAL_ERROR "BANNED_DIRS variable is not set or empty.")
    endif()

    # Convert space-separated to semicolon-separated list
    string(REPLACE " " ";" BANNED_DIRS "${BANNED_DIRS}")

    foreach(dir IN LISTS BANNED_DIRS)
        message(STATUS "Checking for banned functions in: ${dir}")
        file(GLOB_RECURSE found_files
            RELATIVE ${CMAKE_SOURCE_DIR}
            ${dir}/*.c
            ${dir}/*.cpp
            ${dir}/*.h
            ${dir}/*.hpp
        )
        list(APPEND BANNED_SCAN_FILES ${found_files})
    endforeach()

    foreach(file ${BANNED_SCAN_FILES})
        # Read file into lines using file(STRINGS)
        file(STRINGS ${CMAKE_SOURCE_DIR}/${file} file_lines)

        foreach(line IN LISTS file_lines)
            foreach(func ${BANNED_FUNCTIONS})
                if(line MATCHES "\\b${func}\\b")
                    message(FATAL_ERROR "Banned function '${func}' found in ${file}")
                endif()
            endforeach()
            # Only error if the line does not end with //format:allow (allow spaces between // and format)
            # we allow this for specific cases where the format specifier is needed because the
            # string is shown to the user and needs to be formatted in their locale e.g. error messages
            if(line MATCHES "%[0-9]*\\.?[0-9]*[fg]" AND NOT line MATCHES "format:allow")
                message(FATAL_ERROR "Banned format specifier '%f' or '%g' found in ${file}")
            endif()
        endforeach()
    endforeach()
endmacro()

check_banned_functions()