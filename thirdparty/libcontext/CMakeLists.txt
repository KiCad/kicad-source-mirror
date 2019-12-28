# The libcontext library is only included inside common, so we create it as an
# object library and then add the objects to common.

# Link-time optimization (LTO) on GCC conflicts with embedded assembly (__asm),
# following GCC's recommendation to disable LTO per translation unit.
if( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set_source_files_properties( libcontext.cpp PROPERTIES
        COMPILE_FLAGS "-fno-lto"
        )
endif()


add_library( libcontext OBJECT
    libcontext.cpp
    )

target_include_directories( libcontext PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )
