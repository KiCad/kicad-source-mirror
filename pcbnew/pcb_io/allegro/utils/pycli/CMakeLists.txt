


# Add a target to regenerate the Kaitai defs

# find the kaitai compiler
find_program(KAITAI_COMPILER
    NAMES kaitai-struct-compiler ksc
)

if (NOT KAITAI_COMPILER)
    message(STATUS "Kaitai compiler not found, skipping generation of allegro_brd.py")
    return()
endif()

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/allegro_brd.py
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../allegro_brd.ksy
    COMMAND ${KAITAI_COMPILER}
        -t python
        # Keep all the extents for debugging
        --read-pos
        ${CMAKE_SOURCE_DIR}/pcbnew/pcb_io/allegro/utils/allegro_brd.ksy
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(
    allegro_gen_python_defs
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/allegro_brd.py
)
