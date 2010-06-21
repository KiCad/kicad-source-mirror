include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ../potrace
                    ../polygon/kbool/include
                    )

set(BITMAP2COMPONENT_SRCS
    bitmap2component.cpp
    bitmap2cmp_gui_base
    bitmap2cmp_gui
    )

if(WIN32)
    if(MINGW)
        # BITMAP2COMPONENT_RESOURCES variable is set by the macro.
        mingw_resource_compiler(bitmap2component)
    else(MINGW)
        set(BITMAP2COMPONENT_RESOURCES bitmap2component.rc)
    endif(MINGW)
endif(WIN32)
add_executable(bitmap2component WIN32 MACOSX_BUNDLE
                            ${BITMAP2COMPONENT_SRCS}
                            ${BITMAP2COMPONENT_RESOURCES})


target_link_libraries( bitmap2component
                        ${wxWidgets_LIBRARIES}
                        potrace
                        kbool )

install(TARGETS bitmap2component
        DESTINATION ${KICAD_PLUGINS}
        COMPONENT binary)

