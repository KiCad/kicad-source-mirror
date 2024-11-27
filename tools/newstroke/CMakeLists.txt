
find_package(KicadCli)

#
# List of all newstroke symbol files that define the newstroke font
# glyphs. These files are used to generate the newstroke font data
# in newstroke_font.cpp.
#
set(NEWSTROKE_SOURCE_FILES
    font.kicad_sym
    symbol.kicad_sym
    CJK_symbol.kicad_sym
    CJK_wide_U+4E00.kicad_sym
    CJK_wide_U+5AE6.kicad_sym
    CJK_wide_U+66B9.kicad_sym
    CJK_wide_U+7212.kicad_sym
    CJK_wide_U+7D2A.kicad_sym
    CJK_wide_U+8814.kicad_sym
    CJK_wide_U+92B4.kicad_sym
    CJK_wide_U+9C60.kicad_sym
    half_full.kicad_sym
    hiragana.kicad_sym
    katakana.kicad_sym
)

# Make the files absolute (in-place)
list(TRANSFORM NEWSTROKE_SOURCE_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)


add_custom_command(
    OUTPUT
        ${CMAKE_BINARY_DIR}/common/newstroke_font.cpp
    COMMAND
        python3 fontconv.py
    DEPENDS
        ${NEWSTROKE_SOURCE_FILES}
        fontconv.py
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT
        "Generating newstroke font data"
)


add_custom_target(
    newstroke_generate_cpp
    DEPENDS
        ${CMAKE_BINARY_DIR}/common/newstroke_font.cpp
)


if (NOT KICAD_CLI)
    message(STATUS "Cannot create newstroke_upgrade_syms target (kicad-cli not found)")
else()
    add_custom_target(newstroke_upgrade_syms
        COMMENT "Running format update on all newstroke .kicad_sym files"
    )

    KICAD_CLI_UPGRADE_SYMS(
        TARGET newstroke_upgrade_syms
        FILES ${NEWSTROKE_SOURCE_FILES}
    )
endif()

