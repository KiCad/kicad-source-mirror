if(UNIX)
    add_definitions(-D__UNIX__)
endif(UNIX)

add_executable(netlist_form_pads-pcb netlist_form_pads-pcb.cpp)

install(TARGETS netlist_form_pads-pcb
        RUNTIME DESTINATION ${KICAD_PLUGINS}
        COMPONENT binary)
