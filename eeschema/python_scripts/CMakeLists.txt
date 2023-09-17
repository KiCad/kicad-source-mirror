# EESCHEMA's netlist and bom sample scripts

set( PYTHON_SCRIPTS_LIST
    README-bom.txt
    kicad_utils.py
    kicad_netlist_reader.py
    bom_csv_grouped_by_value.py
    bom_csv_grouped_by_value_with_fp.py
    bom_csv_grouped_extra.py
    bom_csv_sorted_by_ref.py
    bom_html_grouped_by_value.py
    bom_html_with_advanced_grouping.py
    bom_txt_sorted_by_ref.py
    )

install( FILES ${PYTHON_SCRIPTS_LIST}
    DESTINATION ${KICAD_PLUGINS}
    COMPONENT binary
    )
