# The libcontext library is only included inside common, so we create it as an
# object library and then add the objects to common.

# Link-time optimization (LTO) on GCC conflicts with embedded assembly (__asm),
# following GCC's recommendation to disable LTO per translation unit.
if( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set_source_files_properties( libcontext.cpp PROPERTIES
        COMPILE_FLAGS "-fno-lto"
        )
endif()

list(APPEND LIBCONTEXT_SOURCES
    libcontext.cpp
    )

if( MSVC )
    if ( KICAD_BUILD_ARCH_X86 )
        list(APPEND LIBCONTEXT_ASM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/make_i386_ms_pe_masm.asm
            ${CMAKE_CURRENT_SOURCE_DIR}/jump_i386_ms_pe_masm.asm
            )
    elseif( KICAD_BUILD_ARCH_X64 )
        list(APPEND LIBCONTEXT_ASM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/make_x86_64_ms_pe_masm.asm
            ${CMAKE_CURRENT_SOURCE_DIR}/jump_x86_64_ms_pe_masm.asm
            )
    elseif( KICAD_BUILD_ARCH_ARM64 )
        list(APPEND LIBCONTEXT_ASM_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/make_arm64_aapcs_pe_armasm.asm
            ${CMAKE_CURRENT_SOURCE_DIR}/jump_arm64_aapcs_pe_armasm.asm
            )
    endif()

    list(APPEND LIBCONTEXT_SOURCES ${LIBCONTEXT_ASM_SOURCES})
endif()

add_library( libcontext STATIC
    ${LIBCONTEXT_SOURCES}
    )

target_include_directories( libcontext PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )
