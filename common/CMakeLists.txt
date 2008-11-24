include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${Boost_INCLUDE_DIR}
                   )
set(COMMON_SRCS
    about_kicad.cpp
    base_screen.cpp
    base_struct.cpp
    basicframe.cpp
    block_commande.cpp
    class_drawpickedstruct.cpp
    common.cpp
    common_plot_functions.cpp
    common_plotHPGL_functions.cpp
    common_plotPS_functions.cpp
    confirm.cpp
    copy_to_clipboard.cpp
    dcsvg.cpp
    displlst.cpp
    dlist.cpp
    drawtxt.cpp
    edaappl.cpp
    eda_dde.cpp
    eda_doc.cpp
    gestfich.cpp
    get_component_dialog.cpp
    gr_basic.cpp
    hotkeys_basic.cpp
    msgpanel.cpp
    projet_config.cpp
#   pyhandler.cpp
    selcolor.cpp
    string.cpp
    toolbars.cpp
    trigo.cpp
    worksheet.cpp
    wxwineda.cpp)

add_library(common ${COMMON_SRCS})
