include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ../potrace
                    ../polygon/kbool/include
                    )

set(BITMAP2COMPONENT_SRCS
    bitmap2component.cpp
    bitmap2cmp_gui_base
    bitmap2cmp_gui
    )

add_executable(bitmap2component WIN32 MACOSX_BUNDLE ${BITMAP2COMPONENT_SRCS} ${BITMAP2COMPONENT_RESOURCES})


target_link_libraries( bitmap2component
                        ${wxWidgets_LIBRARIES}
                        potrace
                        kbool )

install(TARGETS bitmap2component
        DESTINATION ${KICAD_PLUGINS}
        COMPONENT binary)

