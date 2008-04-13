# resource compilation for mingw (http://www.cmake.org/Bug/view.php?id=4068)

macro(dbg_msg _MSG)
#    message(STATUS "${CMAKE_CURRENT_LIST_FILE}(${CMAKE_CURRENT_LIST_LINE}): ${_MSG}")
endmacro(dbg_msg)

macro(mingw_resource_compiler _NAME)
    set(_IN "${CMAKE_CURRENT_SOURCE_DIR}/${_NAME}.rc")
    dbg_msg("_IN: ${_IN}")

    set(_OUT "${CMAKE_CURRENT_BINARY_DIR}/${_NAME}_rc.o")
    dbg_msg("_OUT: ${_OUT}")

    set(_WINDRES_INCLUDE_DIRS -I${CMAKE_CURRENT_SOURCE_DIR})
    foreach(wx_include_dir ${wxWidgets_INCLUDE_DIRS})
        set(_WINDRES_INCLUDE_DIRS ${_WINDRES_INCLUDE_DIRS} -I${wx_include_dir})
    endforeach(wx_include_dir ${wxWidgets_INCLUDE_DIRS})
    dbg_msg("_WINDRES_INCLUDE_DIRS: ${_WINDRES_INCLUDE_DIRS}")

    set(_ARGS ${_WINDRES_INCLUDE_DIRS} -i${_IN} -o${_OUT})
    dbg_msg("_ARGS: ${_ARGS}")

    add_custom_command(OUTPUT ${_OUT}
        COMMAND windres.exe
        ARGS ${_ARGS}
        VERBATIM)
endmacro(mingw_resource_compiler)
