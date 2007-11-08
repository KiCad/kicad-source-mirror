INCLUDE(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(stricmp HAVE_STRICMP)

IF(HAVE_STRICMP)
	ADD_EXECUTABLE(netlist_form_pads-pcb netlist_form_pads-pcb.cpp)

	INSTALL(PROGRAMS netlist_form_pads-pcb DESTINATION /bin)
ENDIF(HAVE_STRICMP)
