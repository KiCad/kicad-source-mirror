# compile_asm(TARGET target ASM_FILES file1 [file2 ...] OUTPUT_OBJECTS [variableName])
# CMake does not support the ARM or ARM64 assemblers on Windows when using the
# MSBuild generator. When the MSBuild generator is in use, we manually compile the assembly files
# using this function.
#
# Borrowed from dotnet/runtime, licensed under MIT
# Copyright (c) .NET Foundation and Contributors
# https://github.com/dotnet/runtime/blob/main/eng/native/functions.cmake
function(compile_asm)
  set(options "")
  set(oneValueArgs TARGET OUTPUT_OBJECTS)
  set(multiValueArgs ASM_FILES)
  cmake_parse_arguments(COMPILE_ASM "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGV})

  get_include_directories_asm(ASM_INCLUDE_DIRECTORIES)

  set (ASSEMBLED_OBJECTS "")

  foreach(ASM_FILE ${COMPILE_ASM_ASM_FILES})
    get_filename_component(name ${ASM_FILE} NAME_WE)
    # Produce object file where CMake would store .obj files for an OBJECT library.
    # ex: artifacts\obj\coreclr\windows.arm64.Debug\src\vm\wks\cee_wks.dir\Debug\AsmHelpers.obj
    set (OBJ_FILE "${CMAKE_CURRENT_BINARY_DIR}/${COMPILE_ASM_TARGET}.dir/${CMAKE_CFG_INTDIR}/${name}.obj")

    # Need to compile asm file using custom command as include directories are not provided to asm compiler
    add_custom_command(OUTPUT ${OBJ_FILE}
                        COMMAND "${CMAKE_ASM_COMPILER}" -g ${ASM_INCLUDE_DIRECTORIES} -o ${OBJ_FILE} ${ASM_FILE}
                        DEPENDS ${ASM_FILE}
                        COMMENT "Assembling ${ASM_FILE} ---> \"${CMAKE_ASM_COMPILER}\" -g ${ASM_INCLUDE_DIRECTORIES} -o ${OBJ_FILE} ${ASM_FILE}")

    # mark obj as source that does not require compile
    set_source_files_properties(${OBJ_FILE} PROPERTIES EXTERNAL_OBJECT TRUE)

    # Add the generated OBJ in the dependency list so that it gets consumed during linkage
    list(APPEND ASSEMBLED_OBJECTS ${OBJ_FILE})
  endforeach()

  set(${COMPILE_ASM_OUTPUT_OBJECTS} ${ASSEMBLED_OBJECTS} PARENT_SCOPE)
endfunction()


# Build a list of include directories for consumption by the assembler
function(get_include_directories_asm IncludeDirectories)
    get_directory_property(dirs INCLUDE_DIRECTORIES)

    foreach(dir IN LISTS dirs)
      list(APPEND INC_DIRECTORIES -I${dir};)
    endforeach()

    set(${IncludeDirectories} ${INC_DIRECTORIES} PARENT_SCOPE)
endfunction(get_include_directories_asm)