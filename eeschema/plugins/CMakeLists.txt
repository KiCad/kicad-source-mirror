INCLUDE(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(stricmp HAVE_STRICMP)

IF(HAVE_STRICMP)
	ADD_EXECUTABLE(netlist_form_pads-pcb netlist_form_pads-pcb.cpp)

	INSTALL(TARGETS netlist_form_pads-pcb RUNTIME DESTINATION ${KICAD_PLUGINS})
ENDIF(HAVE_STRICMP)
