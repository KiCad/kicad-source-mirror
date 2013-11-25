install( FILES
    kicad.pro
    fp-lib-table
    fp-lib-table.csv
    fp-lib-table.for-github
    fp-lib-table.for-eagle-6.4.0
    gost_landscape.kicad_wks
    gost_portrait.kicad_wks
    pagelayout_default.kicad_wks
    pagelayout_logo.kicad_wks

    DESTINATION ${KICAD_TEMPLATE}
    COMPONENT resources
    )
