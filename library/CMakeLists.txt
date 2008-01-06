FILE(GLOB dcm_files "*.dcm")
FILE(GLOB lib_files "*.lib")
FILE(GLOB sym_files "*.sym")

INSTALL(FILES ${dcm_files} DESTINATION ${KICAD_LIBRARY} COMPONENT resources)
INSTALL(FILES ${lib_files} DESTINATION ${KICAD_LIBRARY} COMPONENT resources)
INSTALL(FILES ${sym_files} DESTINATION ${KICAD_LIBRARY} COMPONENT resources)
