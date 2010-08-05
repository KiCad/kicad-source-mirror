if(UNIX)
    add_definitions(-D__UNIX__)
endif(UNIX)

# add_executable(netlist_form_pads-pcb netlist_form_pads-pcb.cpp)
# install(TARGETS netlist_form_pads-pcb   DESTINATION ${KICAD_PLUGINS}  COMPONENT binary)

# install the *.xsl file(s), user will need to install 'xsltproc' and setup
# EESCHEMA's netlist plugins
install( FILES
                netlist_form_pads-pcb.xsl
         DESTINATION ${KICAD_PLUGINS}
         COMPONENT binary
         )
