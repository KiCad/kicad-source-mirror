# resource compilation for mingw (http://www.cmake.org/Bug/view.php?id=4068)

macro(dbg_msg _MSG)
#    message(STATUS "${CMAKE_CURRENT_LIST_FILE}(${CMAKE_CURRENT_LIST_LINE}): ${_MSG}")
endmacro(dbg_msg)

macro(mingw_resource_compiler _NAME)
    # Resource compiler name.
    if(NOT DEFINED CMAKE_RC_COMPILER)
        set(CMAKE_RC_COMPILER windres.exe)
    endif(NOT DEFINED CMAKE_RC_COMPILER)
    dbg_msg("CMAKE_RC_COMPILER: ${CMAKE_RC_COMPILER}")

    # Input file.
    set(_IN "${CMAKE_SOURCE_DIR}/resources/msw/${_NAME}.rc")
    dbg_msg("_IN: ${_IN}")

    # Output file.
    set(_OUT "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${_NAME}.dir/${_NAME}_rc.obj")
    dbg_msg("_OUT: ${_OUT}")

    # Include directories.
    set(_WINDRES_INCLUDE_DIRS -I${CMAKE_CURRENT_SOURCE_DIR})
    foreach(wx_include_dir ${wxWidgets_INCLUDE_DIRS})
        set(_WINDRES_INCLUDE_DIRS ${_WINDRES_INCLUDE_DIRS} -I${wx_include_dir})
    endforeach(wx_include_dir ${wxWidgets_INCLUDE_DIRS})

    foreach(_mingw_rc_include_dir ${mingw_resource_compiler_INCLUDE_DIRS})
        set(_WINDRES_INCLUDE_DIRS ${_WINDRES_INCLUDE_DIRS} -I${_mingw_rc_include_dir})
    endforeach()
    dbg_msg("_WINDRES_INCLUDE_DIRS: ${_WINDRES_INCLUDE_DIRS}")


    foreach(_mingw_rc_define ${mingw_resource_compiler_DEFINES})
        set(_WINDRES_DEFINES ${_WINDRES_DEFINES} -D${_mingw_rc_define})
    endforeach()
    dbg_msg("_WINDRES_DEFINES: ${_WINDRES_DEFINES}")

    # windres arguments.
    set(_ARGS ${_WINDRES_INCLUDE_DIRS} ${_WINDRES_DEFINES} -i${_IN} -o${_OUT})
    dbg_msg("_ARGS: ${_ARGS}")

    # Compile resource file.
    add_custom_command(OUTPUT ${_OUT}
        COMMAND ${CMAKE_RC_COMPILER}
        ARGS ${_ARGS}
        COMMENT "Compiling ${_NAME}'s resource file"
        VERBATIM)

    # Set a NAME_RESOURCES variable
    string(TOUPPER ${_NAME} _NAME_UPPER)
    set(${_NAME_UPPER}_RESOURCES ${_OUT})
    dbg_msg("${_NAME_UPPER}_RESOURCES: ${${_NAME_UPPER}_RESOURCES}")
endmacro(mingw_resource_compiler)
