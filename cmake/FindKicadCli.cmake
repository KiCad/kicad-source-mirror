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
#   FILE    - The symbol library file to upgrade
#   TARGET  - The CMake target to add the upgrade command to
#   FORCE   - Optional argument to force the upgrade
#
function(KICAD_CLI_UPGRADE_SYMS FILE TARGET)
    if (NOT KICAD_CLI)
        message(FATAL_ERROR "Cannot run upgrade target (kicad-cli not found)")
    endif()

    # Parse the optional FORCE argument
    set(options FORCE)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Check if FORCE was provided
    if (ARGS_FORCE)
        set(FORCE_ARG "--force")
    else()
        set(FORCE_ARG "")
    endif()

    add_custom_command(
        TARGET ${TARGET}
        COMMAND ${KICAD_CLI} sym upgrade ${FORCE_ARG} ${FILE}
        DEPENDS ${FILE}
        COMMENT
            "Upgrading symbol lib format: ${FILE}"
    )
endfunction()