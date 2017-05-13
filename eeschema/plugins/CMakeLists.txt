# Install the *.xsl file(s), user will need to install 'xsltproc' and setup
# EESCHEMA's netlist plugins
# See chapter 14 of eeschema.pdf


set( XSL_SCRIPTS_LIST
    xsl_scripts/bom2csv.xsl
    xsl_scripts/bom2grouped_csv.xsl
    xsl_scripts/netlist_form_cadstar-RINF.xsl
    xsl_scripts/netlist_form_cadstar.xsl
    xsl_scripts/netlist_form_OrcadPcb2.xsl
    xsl_scripts/netlist_form_pads-pcb.xsl
    xsl_scripts/bom_with_title_block_2_csv.xsl
    )

set( PYTHON_SCRIPTS_LIST
    python_scripts/README-bom.txt
    python_scripts/kicad_netlist_reader.py
    python_scripts/bom_csv_grouped_by_value.py
    python_scripts/bom_csv_grouped_by_value_with_fp.py
    python_scripts/bom_csv_sorted_by_ref.py
    python_scripts/bom_html_grouped_by_value.py
    python_scripts/bom_html_with_advanced_grouping.py
    python_scripts/bom_sorted_by_ref.py
    )

install( FILES ${XSL_SCRIPTS_LIST} ${PYTHON_SCRIPTS_LIST}
    DESTINATION ${KICAD_PLUGINS}
    COMPONENT binary
    )
