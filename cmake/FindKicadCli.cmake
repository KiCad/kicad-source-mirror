# - Find kicad-cli
# Find the kicad-cli tool
#
# Used for managing KiCad files in the repo (e.g. test
# files, stroke font files,

find_program(KICAD_CLI
    NAMES kicad-cli
)

if (KICAD_CLI)
    message(STATUS "kicad-cli found: ${KICAD_CLI}")
else()
    message(STATUS "kicad-cli not found")
endif()


#
# Helper function to upgrade a symbol library file
# using the installed kicad-cli tool
#
# Usage:
#   KICAD_CLI_UPGRADE_SYMS(FILE TARGET [FORCE])
#
# Arguments:
#   FILES   - The symbol library file to upgrade
#   TARGET  - The CMake target to add the upgrade command to
#   FORCE   - Optional argument to force the upgrade
#
function(KICAD_CLI_UPGRADE_SYMS)
    if (NOT KICAD_CLI)
        message(FATAL_ERROR "Cannot run upgrade target (kicad-cli not found)")
    endif()

    # Parse the optional FORCE argument
    set(options FORCE)
    set(oneValueArgs TARGET)
    set(multiValueArgs FILES)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Check if FORCE was provided
    if (ARGS_FORCE)
        set(FORCE_ARG "--force")
    else()
        set(FORCE_ARG "")
    endif()

    # Validate required arguments
    if(NOT ARGS_TARGET)
        message(FATAL_ERROR "KICAD_CLI_UPGRADE_SYMS requires a TARGET argument")
    endif()

    foreach(FILE ${ARGS_FILES})
        add_custom_command(
            TARGET ${ARGS_TARGET}
            PRE_BUILD
            COMMAND ${KICAD_CLI} sym upgrade ${FORCE_ARG} ${FILE}
            COMMENT
                "Upgrading symbol lib format: ${FILE}"
        )
    endforeach()

endfunction()