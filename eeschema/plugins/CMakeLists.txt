IF(UNIX)
	ADD_DEFINITIONS(-D__UNIX__)
ENDIF(UNIX)

ADD_EXECUTABLE(netlist_form_pads-pcb netlist_form_pads-pcb.cpp)

INSTALL(TARGETS netlist_form_pads-pcb RUNTIME DESTINATION ${KICAD_PLUGINS} COMPONENT binary)
