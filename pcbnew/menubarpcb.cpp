/******************************************************************/
/* menubarpcb.cpp - creation du menu general de l'editeur de board*/
/******************************************************************/
#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "bitmaps.h"
#include "protos.h"
#include "hotkeys.h"
#include "pcbnew_id.h"

/***********************************************/
void WinEDA_PcbFrame::ReCreateMenuBar()
/***********************************************/

/* Cree ou reinitialise le menu du haut d'ecran
 */
{
    wxMenuItem* item;
    wxMenuBar*  menuBar = GetMenuBar();

    /* Destroy the existing menu bar so it can be rebuilt.  This allows
     * language changes of the menu text on the fly. */
    if( menuBar )
        SetMenuBar( NULL );

    menuBar = new wxMenuBar();
    wxMenu* filesMenu = new wxMenu;

    // New board
    item = new wxMenuItem( filesMenu, ID_NEW_BOARD, _( "&New Board" ),
                           _( "Clear old board and initialize a new one" ) );
    item->SetBitmap( new_xpm );
    filesMenu->Append( item );

    // Load board
    item = new wxMenuItem( filesMenu, ID_LOAD_FILE, _( "&Load Board" ),
                           _( "Delete old board and load new board" ) );
    item->SetBitmap( open_xpm );
    filesMenu->Append( item );

    // Append board
    item = new wxMenuItem( filesMenu, ID_APPEND_FILE, _( "Append Board" ),
                           _( "Add board to old board" ) );
    item->SetBitmap( import_xpm );
    filesMenu->Append( item );


    item = new wxMenuItem( filesMenu, ID_MENU_RECOVER_BOARD, _( "&Rescue" ),
                           _( "Clear old board and get last rescue file" ) );
    item->SetBitmap( hammer_xpm );
    filesMenu->Append( item );

    item = new wxMenuItem( filesMenu, ID_MENU_READ_LAST_SAVED_VERSION_BOARD,
                           _( "&Previous Version" ),
                           _( "Clear old board and get old version of board" ) );
    item->SetBitmap( jigsaw_xpm );
    filesMenu->Append( item );

    // Add save menu
    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, ID_SAVE_BOARD,
                           _( "&Save Board\tCtrl-S" ), _( "Save current board" ) );
    item->SetBitmap( save_xpm );
    filesMenu->Append( item );

    item = new wxMenuItem( filesMenu, ID_SAVE_BOARD_AS,
                           _( "Save Board as..." ),
                           _( "Save current board as.." ) );
    item->SetBitmap( save_as_xpm );
    filesMenu->Append( item );

    // Add print menu
    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, ID_GEN_PRINT, _( "P&rint" ),
                           _( "Print pcb board" ) );
    item->SetBitmap( print_button );
    filesMenu->Append( item );

    item = new wxMenuItem( filesMenu, ID_GEN_PLOT_SVG, _( "Print S&VG" ),
                           _( "Plot pcb board in SVG format" ) );
    item->SetBitmap( print_button );
    filesMenu->Append( item );

    // Add plot menu
    item = new wxMenuItem( filesMenu, ID_GEN_PLOT, _( "&Plot" ),
                          _( "Plot (HPGL, PostScript, or Gerber format)" ) );
    item->SetBitmap( plot_xpm );
    filesMenu->Append( item );

    // Add Export menu
    filesMenu->AppendSeparator();
    wxMenu* submenuexport = new wxMenu();

    item = new wxMenuItem( submenuexport, ID_GEN_EXPORT_SPECCTRA,
                           _( "&Specctra DSN" ),
                          _( "Export the current board to a \"Specctra DSN\" file" ) );
    item->SetBitmap( export_xpm );
    submenuexport->Append( item );

    item = new wxMenuItem( submenuexport, ID_GEN_EXPORT_FILE_GENCADFORMAT,
                           _( "&GenCAD" ), _( "Export GenCAD Format" ) );
    item->SetBitmap( export_xpm );
    submenuexport->Append( item );

    item = new wxMenuItem( submenuexport, ID_GEN_EXPORT_FILE_MODULE_REPORT,
                           _( "&Module Report" ),
                           _( "Create a board report (footprint report)" ) );
    item->SetBitmap( tools_xpm );
    submenuexport->Append( item );
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, submenuexport,
                                        ID_GEN_EXPORT_FILE, _( "&Export" ),
                                        _( "Export board" ), export_xpm );


    //-----<Add import menu>-----------------------------------------------
    // no separator, keep it next to Import
    wxMenu* submenuImport = new wxMenu();

    item = new wxMenuItem( submenuImport, ID_GEN_IMPORT_SPECCTRA_SESSION,
                           _( "&Specctra Session" ),
                           _( "Import a routed \"Specctra Session\" (*.ses) file" ) );
    item->SetBitmap( import_xpm );    // @todo need better bitmap
    submenuImport->Append( item );

    /* would be implemented in WinEDA_PcbFrame::ImportSpecctraDesign() in specctra_import.cpp
     *  item = new wxMenuItem(submenuImport, ID_GEN_IMPORT_SPECCTRA_DESIGN,
     *  _("&Specctra Design"), _("Import a \"Specctra Design\" (*.dsn) file") );
     *  item->SetBitmap(export_xpm);    // @todo need better bitmap
     *  submenuImport->Append(item);
     */

    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, submenuImport,
                                        ID_GEN_IMPORT_FILE, _( "Import" ),
                                        _( "Import files" ), import_xpm );

    //-----</Add import menu>----------------------------------------------

    // Add archive footprints menu
    filesMenu->AppendSeparator();
    wxMenu* submenuarchive = new wxMenu();
    item = new wxMenuItem( submenuarchive, ID_MENU_ARCHIVE_NEW_MODULES,
                           _( "Add New Footprints" ),
                           _( "Archive new footprints only in a library (keep other footprints in this lib)" ) );
    item->SetBitmap( library_update_xpm );
    submenuarchive->Append( item );
    item = new wxMenuItem( submenuarchive, ID_MENU_ARCHIVE_ALL_MODULES,
                           _( "Create Footprint Archive" ),
                           _( "Archive all footprints  in a library(old lib will be deleted)" ) );
    item->SetBitmap( library_xpm );
    submenuarchive->Append( item );
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, submenuarchive,
                                        ID_MENU_ARCHIVE_MODULES,
                                        _( "Archive Footprints" ),
                                        _( "Archive or add footprints in a library file" ),
                                        library_xpm );

    // Add exit menu
    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, ID_EXIT, _( "E&xit" ),
                           _( "Quit PCBNEW" ) );
    item->SetBitmap( exit_xpm );
    filesMenu->Append( item );

    wxGetApp().m_fileHistory.AddFilesToMenu( filesMenu );

    ///////////////////////////////////
    // Configuration et preferences: //
    ///////////////////////////////////
    wxMenu* configmenu = new wxMenu;
    item = new wxMenuItem( configmenu, ID_CONFIG_REQ, _( "&Library" ),
                           _( "Setting libraries, directories and others..." ) );
    item->SetBitmap( library_xpm );
    configmenu->Append( item );

    item = new wxMenuItem( configmenu, ID_COLORS_SETUP, _( "&Colors and Visibility" ),
                           _( "Select colors and visibilty of layers and some items" ) );
    item->SetBitmap( palette_xpm );
    configmenu->Append( item );

    item = new wxMenuItem( configmenu, ID_OPTIONS_SETUP, _( "&General" ),
                           _( "Select general options for PCBNEW" ) );
    item->SetBitmap( preference_xpm );
    configmenu->Append( item );

    item = new wxMenuItem( configmenu, ID_PCB_DISPLAY_OPTIONS_SETUP, _( "&Display" ),
                           _( "Select how items (pads, tracks texts ... ) are displayed" ) );
    item->SetBitmap( display_options_xpm );
    configmenu->Append( item );

    wxGetApp().AddMenuLanguageList( configmenu );

    configmenu->AppendSeparator();
    item = new wxMenuItem( configmenu, ID_CONFIG_SAVE, _( "&Save Preferences" ),
                           _( "Save application preferences" ) );
    item->SetBitmap( save_setup_xpm );
    configmenu->Append( item );

    item = new wxMenuItem( configmenu, ID_CONFIG_READ, _( "&Read Preferences" ),
                           _( "Read application preferences" ) );
    item->SetBitmap( read_setup_xpm );
    configmenu->Append( item );

    configmenu->AppendSeparator();
    AddHotkeyConfigMenu( configmenu );

    // Add access to the Design Rules Dialog:
    wxMenu* designRulesMenu = new wxMenu;
    item = new wxMenuItem( designRulesMenu, ID_MENU_PCB_SHOW_DESIGN_RULES_DIALOG,
                           _( "Design Rules" ), _( "Open the design rules dialog editor" ) );
    item->SetBitmap( hammer_xpm );
    designRulesMenu->Append( item );

/*
    item = new wxMenuItem( designRulesMenu, ID_PCB_COPPER_LAYERS_SETUP, _( "Copper &Layers" ),
                           _( "Select copper layers count and layers names" ) );
    item->SetBitmap( copper_layers_setup_xpm );
    designRulesMenu->Append( item );
*/

    item = new wxMenuItem( configmenu, ID_PCB_LAYERS_SETUP, _( "&Layers Setup" ),
                           _( "Enable and set properties of layers" ) );
    item->SetBitmap( copper_layers_setup_xpm );
    designRulesMenu->Append( item );


    /////////////////////////////
    // Ajustage de dimensions: //
    /////////////////////////////
    wxMenu* sizes_menu = new wxMenu;

    item = new wxMenuItem( sizes_menu, ID_PCB_TRACK_SIZE_SETUP,
                           _( "Tracks and Vias" ),
                           _( "Adjust size and width for tracks and vias" ) );
    item->SetBitmap( showtrack_xpm );
    sizes_menu->Append( item );

    item = new wxMenuItem( sizes_menu, ID_PCB_USER_GRID_SETUP, _( "Grid" ),
                           _( "Adjust User Grid" ) );
    item->SetBitmap( grid_xpm );
    sizes_menu->Append( item );

    item = new wxMenuItem( sizes_menu, ID_PCB_DRAWINGS_WIDTHS_SETUP,
                           _( "Texts and Drawings" ),
                           _( "Adjust width for texts and drawings" ) );
    item->SetBitmap( options_text_xpm );
    sizes_menu->Append( item );

    item = new wxMenuItem( sizes_menu, ID_PCB_PAD_SETUP, _( "Pads" ),
                           _( "Adjust size,shape,layers... for pads" ) );
    item->SetBitmap( pad_xpm );
    sizes_menu->Append( item );

    sizes_menu->AppendSeparator();
    item = new wxMenuItem( sizes_menu, ID_CONFIG_SAVE, _( "&Save Setup" ),
                           _( "Save options in current directory" ) );
    item->SetBitmap( save_xpm );
    sizes_menu->Append( item );

    //////////////////////////////////////////////////////////////////
    // Menu postprocess ( generation fichiers percage, placement... //
    //////////////////////////////////////////////////////////////////
    wxMenu* postprocess_menu = new wxMenu;
    item = new wxMenuItem( postprocess_menu, ID_PCB_GEN_POS_MODULES_FILE,
                           _( "Generate &Modules Position" ),
                           _( "Generate modules position file" ) );
    item->SetBitmap( post_compo_xpm );
    postprocess_menu->Append( item );

    item = new wxMenuItem( postprocess_menu, ID_PCB_GEN_DRILL_FILE,
                           _( "Create &Drill File" ),
                           _( "Generate excellon2 drill file" ) );
    item->SetBitmap( post_drill_xpm );
    postprocess_menu->Append( item );

    item = new wxMenuItem( postprocess_menu, ID_PCB_GEN_CMP_FILE,
                           _( "Create &Component File" ),
                           _( "Recreate .cmp file for CvPcb" ) );
    item->SetBitmap( save_cmpstuff_xpm );
    postprocess_menu->Append( item );

    item = new wxMenuItem( postprocess_menu, ID_PCB_GEN_BOM_FILE_FROM_BOARD,
                           _( "Create &BOM File" ),
                           _( "Recreate .csv file for CvPcb" ) );
    item->SetBitmap( tools_xpm );
    postprocess_menu->Append( item );

    //////////////////////////
    // Menu d'outils divers //
    //////////////////////////
    wxMenu* miscellaneous_menu = new wxMenu;
    item = new wxMenuItem( miscellaneous_menu, ID_PCB_GLOBAL_DELETE,
                           _( "Global &Deletions" ),
                           _( "Delete tracks, modules, texts... on board" ) );
    item->SetBitmap( general_deletions_xpm );
    miscellaneous_menu->Append( item );

    item =
        new wxMenuItem( miscellaneous_menu, ID_MENU_LIST_NETS, _( "&List Nets" ),
                        _( "List nets (names and id)" ) );
    item->SetBitmap( tools_xpm );
    miscellaneous_menu->Append( item );

    item = new wxMenuItem( miscellaneous_menu, ID_MENU_PCB_CLEAN,
                           _( "&Track Operations" ),
                           _( "Clean stubs, vias, delete break points, or connect dangling tracks to pads and vias" ) );
    item->SetBitmap( delete_body_xpm );
    miscellaneous_menu->Append( item );

    item = new wxMenuItem( miscellaneous_menu, ID_MENU_PCB_SWAP_LAYERS,
                           _( "&Swap Layers" ),
                           _( "Swap tracks on copper layers or drawings on others layers" ) );
    item->SetBitmap( swap_layer_xpm );
    miscellaneous_menu->Append( item );

    ////////////////
    // Menu Help: //
    ////////////////
    wxMenu* helpMenu = new wxMenu;
    item = new wxMenuItem( helpMenu, ID_GENERAL_HELP, _( "&Contents" ),
                           _( "Open the PCBNEW manual" ) );
    item->SetBitmap( help_xpm );
    helpMenu->Append( item );

    item = new wxMenuItem( helpMenu, ID_KICAD_ABOUT, _( "&About PCBNEW" ),
                           _( "About PCBNEW printed circuit board designer" ) );
    item->SetBitmap( info_xpm );
    helpMenu->Append( item );

    //////////////////////
    // Menu Display 3D: //
    //////////////////////
    wxMenu* Display3DMenu = new wxMenu;
    item = new wxMenuItem( Display3DMenu, ID_MENU_PCB_SHOW_3D_FRAME,
                           _( "3D Display" ), _( "Show board in 3D viewer" ) );
    item->SetBitmap( show_3d_xpm );
    Display3DMenu->Append( item );

    menuBar->Append( filesMenu, _( "&File" ) );
    menuBar->Append( configmenu, _( "&Preferences" ) );
    menuBar->Append( designRulesMenu, _( "&Design Rules" ) );
    menuBar->Append( sizes_menu, _( "&Dimensions" ) );
    menuBar->Append( miscellaneous_menu, _( "&Miscellaneous" ) );
    menuBar->Append( postprocess_menu, _( "P&ostprocess" ) );
    menuBar->Append( Display3DMenu, _( "&3D Display" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    // Associate the menu bar with the frame
    SetMenuBar( menuBar );
}
