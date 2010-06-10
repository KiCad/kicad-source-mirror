include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ../potrace
                    ../polygon/kbool/include
                    )

set(BITMAP2COMPONENT_SRCS
    bitmap2component.cpp
    )

add_executable(bitmap2component WIN32 MACOSX_BUNDLE ${BITMAP2COMPONENT_SRCS} ${BITMAP2COMPONENT_RESOURCES})


target_link_libraries( bitmap2component potrace kbool )

if(APPLE)
    set_target_properties(bitmap2component PROPERTIES )
endif(APPLE)

install(TARGETS bitmap2component
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)

