# EESCHEMA's netlist and bom sample scripts

set( PYTHON_SCRIPTS_LIST
    python_scripts/README-bom.txt
    python_scripts/kicad_utils.py
    python_scripts/kicad_netlist_reader.py
    python_scripts/bom_csv_grouped_by_value.py
    python_scripts/bom_csv_grouped_by_value_with_fp.py
    python_scripts/bom_csv_grouped_extra.py
    python_scripts/bom_csv_sorted_by_ref.py
    python_scripts/bom_html_grouped_by_value.py
    python_scripts/bom_html_with_advanced_grouping.py
    python_scripts/bom_txt_sorted_by_ref.py
    )

install( FILES ${PYTHON_SCRIPTS_LIST}
    DESTINATION ${KICAD_PLUGINS}
    COMPONENT binary
    )
