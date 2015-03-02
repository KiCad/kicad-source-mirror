# Install the *.xsl file(s), user will need to install 'xsltproc' and setup
# EESCHEMA's netlist plugins
# See chapter 14 of eeschema.pdf


set( xsl_lst
    bom2csv.xsl
    bom_cvs.xsl
    netlist_form_cadstar-RINF.xsl
    netlist_form_cadstar.xsl
    netlist_form_OrcadPcb2.xsl
    netlist_form_pads-pcb.xsl
    bom_with_title_block_2_csv.xsl
    )

install( FILES ${xsl_lst}
    DESTINATION ${KICAD_PLUGINS}
    COMPONENT binary
    )
