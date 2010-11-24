/**
 * @file menubarpcb.cpp
 * PCBNew editor menu bar
 */
#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "bitmaps.h"
#include "protos.h"
#include "hotkeys.h"
#include "pcbnew_id.h"

#include "help_common_strings.h"

/**
 * PCBNew mainframe menubar
 */
void WinEDA_PcbFrame::ReCreateMenuBar()
{
    wxString    text;
    wxMenuItem* item;
    wxMenuBar*  menuBar = GetMenuBar();

    /**
     * Destroy the existing menu bar so it can be rebuilt.  This allows
     * language changes of the menu text on the fly.
     */
    if( menuBar )
        SetMenuBar( NULL );
    menuBar = new wxMenuBar();


    /** Create File Menu */
    wxMenu* filesMenu = new wxMenu;

    // New
    item = new wxMenuItem( filesMenu, ID_NEW_BOARD,
                          _( "&New" ),
                          _( "Clear current board and initialize a new one" ) );
    item->SetBitmap( new_xpm );
    filesMenu->Append( item );

    // Open
    item = new wxMenuItem( filesMenu, ID_LOAD_FILE,
                           _( "&Open\tCtrl+O" ),
                           _( "Delete current board and load new board" ) );
    item->SetBitmap( open_xpm );
    filesMenu->Append( item );

    // Load Recent submenu
    wxMenu* openRecentMenu = new wxMenu();
    wxGetApp().m_fileHistory.AddFilesToMenu( openRecentMenu );
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, openRecentMenu,
                                        -1, _( "Open &Recent" ),
                                        _( "Open a recent opened board" ),
                                        open_project_xpm );


    // PCBNew Board
    item = new wxMenuItem( filesMenu, ID_APPEND_FILE,
               _( "&Append Board" ),
               _( "Append another PCBNew board to the current loaded board" ) );
    item->SetBitmap( import_xpm );
    filesMenu->Append( item );

    // Separator
    filesMenu->AppendSeparator();

    // Save
    item = new wxMenuItem( filesMenu, ID_SAVE_BOARD,
                           _( "&Save\tCtrl+S" ),
                           _( "Save current board" ) );
    item->SetBitmap( save_xpm );
    filesMenu->Append( item );

    // Save As
    item = new wxMenuItem( filesMenu, ID_SAVE_BOARD_AS,
                          _( "Save as..." ),
                          _( "Save the current board as.." ) );
    item->SetBitmap( save_as_xpm );
    filesMenu->Append( item );
    filesMenu->AppendSeparator();

    // Revert
    item = new wxMenuItem( filesMenu, ID_MENU_READ_LAST_SAVED_VERSION_BOARD,
                           _( "&Revert" ),
                           _( "Clear board and get previous saved version of board" ) );
    item->SetBitmap( jigsaw_xpm );
    filesMenu->Append( item );

    // Rescue
    item = new wxMenuItem( filesMenu, ID_MENU_RECOVER_BOARD, _( "&Rescue" ),
                           _( "Clear old board and get last rescue file" ) );
    item->SetBitmap( hammer_xpm );
    filesMenu->Append( item );
    filesMenu->AppendSeparator();


    /* Fabrication Outputs submenu */
    wxMenu* fabricationOutputsMenu = new wxMenu;
    item = new wxMenuItem( fabricationOutputsMenu, ID_PCB_GEN_POS_MODULES_FILE,
                           _( "&Modules Position File" ),
                           _( "Generate modules position file for pick and place" ) );
    item->SetBitmap( post_compo_xpm );
    fabricationOutputsMenu->Append( item );

    item = new wxMenuItem( fabricationOutputsMenu, ID_PCB_GEN_DRILL_FILE,
                           _( "&Drill File" ),
                           _( "Generate excellon2 drill file" ) );
    item->SetBitmap( post_drill_xpm );
    fabricationOutputsMenu->Append( item );

    // Component File
    item = new wxMenuItem( fabricationOutputsMenu, ID_PCB_GEN_CMP_FILE,
                           _( "&Component File" ),
                           _( "(Re)create components file (*.cmp) for CvPcb" ) );
    item->SetBitmap( save_cmpstuff_xpm );
    fabricationOutputsMenu->Append( item );

    // BOM File
    item = new wxMenuItem( fabricationOutputsMenu, ID_PCB_GEN_BOM_FILE_FROM_BOARD,
                           _( "&BOM File" ),
                           _( "Create a bill of materials from schematic" ) );
    item->SetBitmap( tools_xpm );
    fabricationOutputsMenu->Append( item );

    /* Fabrications Outputs submenu append */
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, fabricationOutputsMenu,
                                        -1, _( "Fabrication Outputs" ),
                                        _( "Generate files for fabrication" ),
                                        fabrication_xpm );

    /* Import submenu */
    wxMenu* submenuImport = new wxMenu();

    // Specctra Session
    item = new wxMenuItem( submenuImport, ID_GEN_IMPORT_SPECCTRA_SESSION,
                           _( "&Specctra Session" ),
                           _( "Import a routed \"Specctra Session\" (*.ses) file" ) );
    item->SetBitmap( import_xpm );    // @todo need better bitmap
    submenuImport->Append( item );

    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, submenuImport,
                                        ID_GEN_IMPORT_FILE, _( "Import" ),
                                        _( "Import files" ), import_xpm );

    /* Export submenu */
    wxMenu* submenuexport = new wxMenu();

    // Specctra DSN
    item = new wxMenuItem( submenuexport, ID_GEN_EXPORT_SPECCTRA,
                           _( "&Specctra DSN" ),
                           _( "Export the current board to a \"Specctra DSN\" file" ) );
    item->SetBitmap( export_xpm );
    submenuexport->Append( item );

    // GenCAD
    item = new wxMenuItem( submenuexport, ID_GEN_EXPORT_FILE_GENCADFORMAT,
                           _( "&GenCAD" ), _( "Export GenCAD format" ) );
    item->SetBitmap( export_xpm );
    submenuexport->Append( item );

    // Module Report
    item = new wxMenuItem( submenuexport, ID_GEN_EXPORT_FILE_MODULE_REPORT,
                           _( "&Module Report" ),
                           _( "Create a report of all modules on the current board" ) );
    item->SetBitmap( tools_xpm );
    submenuexport->Append( item );

    // VRML
    item = new wxMenuItem( submenuexport, ID_GEN_EXPORT_FILE_VRML,
                           _( "&VRML" ),
                           _( "Export a VRML board representation" ) );
    item->SetBitmap( show_3d_xpm );
    submenuexport->Append( item );

    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, submenuexport,
                                        ID_GEN_EXPORT_FILE, _( "&Export" ),
                                        _( "Export board" ), export_xpm );

    filesMenu->AppendSeparator();

    // Print
    item = new wxMenuItem( filesMenu, wxID_PRINT,
                           _( "&Print\tCtrl+P" ),
                           _( "Print board" ) );
    item->SetBitmap( print_button );
    filesMenu->Append( item );

    // Create SVG file
    item = new wxMenuItem( filesMenu, ID_GEN_PLOT_SVG,
                           _( "Print S&VG" ),
                           _( "Plot board in Scalable Vector Graphics format" ) );
    item->SetBitmap( print_button );
    filesMenu->Append( item );

    // Plot
    item = new wxMenuItem( filesMenu, ID_GEN_PLOT,
              _( "&Plot" ),
              _( "Plot board in HPGL, PostScript or Gerber RS-274X format)" ) );
    item->SetBitmap( plot_xpm );
    filesMenu->Append( item );
   filesMenu->AppendSeparator();

    wxMenu* submenuarchive = new wxMenu();

    // Archive New Footprints
    item = new wxMenuItem( submenuarchive, ID_MENU_ARCHIVE_NEW_MODULES,
                           _( "Archive New Footprints" ),
                           _( "Archive new footprints only in a library (keep other footprints in this lib)" ) );
    item->SetBitmap( library_update_xpm );
    submenuarchive->Append( item );

    // Create FootPrint Archive
    item = new wxMenuItem( submenuarchive, ID_MENU_ARCHIVE_ALL_MODULES,
                           _( "Create Footprint Archive" ),
                           _( "Archive all footprints in a library (old library will be deleted)" ) );
    item->SetBitmap( library_xpm );
    submenuarchive->Append( item );

    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, submenuarchive,
                                        ID_MENU_ARCHIVE_MODULES,
                                        _( "Archive Footprints" ),
                                        _( "Archive or add footprints in a library file" ),
                                        library_xpm );

    /* Quit */
#if !defined( __WXMAC__ )

    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, wxID_EXIT, _( "&Quit" ), _( "Quit PCBNew" ) );
    item->SetBitmap( exit_xpm );
    filesMenu->Append( item );

#endif /* !defined( __WXMAC__ ) */

    /** Create Edit menu **/
    wxMenu* editMenu = new wxMenu;

    // Undo
    text  = AddHotkeyName( _( "Undo" ), g_Pcbnew_Editor_Hokeys_Descr, HK_UNDO );
    item = new wxMenuItem( editMenu, wxID_UNDO, text,
                           HELP_UNDO, wxITEM_NORMAL );
    item->SetBitmap( undo_xpm );
    editMenu->Append( item );

    // Redo
    text  = AddHotkeyName( _( "Redo" ), g_Pcbnew_Editor_Hokeys_Descr, HK_REDO );
    item = new wxMenuItem( editMenu, wxID_REDO, text,
                           HELP_REDO, wxITEM_NORMAL );
    item->SetBitmap( redo_xpm );
    editMenu->Append( item );

    // Delete
    item = new wxMenuItem( editMenu, ID_PCB_DELETE_ITEM_BUTT,
                           _( "Delete" ),
                           _( "Delete items" ) );
    item->SetBitmap( delete_body_xpm );
    editMenu->Append( item );
    editMenu->AppendSeparator();

    // Find
    text = AddHotkeyName( _( "&Find" ), g_Pcbnew_Editor_Hokeys_Descr, HK_FIND_ITEM );
    item = new wxMenuItem( editMenu, ID_FIND_ITEMS,
                           text, HELP_FIND );
    item->SetBitmap( find_xpm );
    editMenu->Append( item );
    editMenu->AppendSeparator();

    // Global Deletions
    item = new wxMenuItem( editMenu, ID_PCB_GLOBAL_DELETE,
                           _( "Global &Deletions" ),
                           _( "Delete tracks, modules, texts... on board" ) );
    item->SetBitmap( general_deletions_xpm );
    editMenu->Append( item );

    // Cleanup Tracks and Vias
    item = new wxMenuItem( editMenu, ID_MENU_PCB_CLEAN,
                           _( "&Cleanup Tracks and Vias" ),
                           _( "Clean stubs, vias, delete break points, or connect dangling tracks to pads and vias" ) );
    item->SetBitmap( delete_body_xpm );
    editMenu->Append( item );

    // Swap Layers
    item = new wxMenuItem( editMenu, ID_MENU_PCB_SWAP_LAYERS,
                           _( "&Swap Layers" ),
                           _( "Swap tracks on copper layers or drawings on other layers" ) );
    item->SetBitmap( swap_layer_xpm );
    editMenu->Append( item );


    /** Create View menu **/
    wxMenu* viewMenu = new wxMenu;

    /* Important Note for ZOOM IN and ZOOM OUT commands from menubar:
     * we cannot add hotkey info here, because the hotkey HK_ZOOM_IN and HK_ZOOM_OUT
     * events(default = WXK_F1 and WXK_F2) are *NOT* equivalent to this menu command:
     * zoom in and out from hotkeys are equivalent to the pop up menu zoom
     * From here, zooming is made around the screen center
     * From hotkeys, zooming is made around the mouse cursor position
     * (obviously not possible from the toolbar or menubar command)
     *
     * in other words HK_ZOOM_IN and HK_ZOOM_OUT *are NOT* accelerators
     * for Zoom in and Zoom out sub menus
     */
    // Zoom In
    text = AddHotkeyName( _( "Zoom In" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ZOOM_IN, false );
    item = new wxMenuItem( viewMenu, ID_ZOOM_IN, text,
                           HELP_ZOOM_IN, wxITEM_NORMAL );
    item->SetBitmap( zoom_in_xpm );
    viewMenu->Append( item );

    // Zoom Out
    text = AddHotkeyName( _( "Zoom Out" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ZOOM_OUT, false );
    item = new wxMenuItem( viewMenu, ID_ZOOM_OUT, text,
                           HELP_ZOOM_OUT, wxITEM_NORMAL );

    item->SetBitmap( zoom_out_xpm );
    viewMenu->Append( item );

    // Fit on Screen
    text = AddHotkeyName( _( "Fit on Screen" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ZOOM_AUTO );

    item = new wxMenuItem( viewMenu, ID_ZOOM_PAGE, text,
                           HELP_ZOOM_FIT, wxITEM_NORMAL );
    item->SetBitmap( zoom_auto_xpm );
    viewMenu->Append( item );

    viewMenu->AppendSeparator();

    // Redraw
    text = AddHotkeyName( _( "Redraw" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ZOOM_REDRAW );

    item = new wxMenuItem( viewMenu, ID_ZOOM_REDRAW, text,
                           HELP_ZOOM_REDRAW,
                           wxITEM_NORMAL );
    item->SetBitmap( zoom_redraw_xpm );
    viewMenu->Append( item );
    viewMenu->AppendSeparator();

    // 3D Display
    item = new wxMenuItem( viewMenu, ID_MENU_PCB_SHOW_3D_FRAME,
                           _( "3D Display" ),
                           _( "Show board in 3D viewer" ) );
    item->SetBitmap( show_3d_xpm );
    viewMenu->Append( item );

    // List Nets
    item = new wxMenuItem( viewMenu, ID_MENU_LIST_NETS,
                           _( "&List Nets" ),
                           _( "View a list of nets with names and id's" ) );
    item->SetBitmap( tools_xpm );
    viewMenu->Append( item );



    /** Create Place Menu **/
    wxMenu* placeMenu = new wxMenu;

    // Module
    text = AddHotkeyName( _( "Module" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ADD_MODULE, false );
    item = new wxMenuItem( placeMenu, ID_COMPONENT_BUTT, text,
                           _( "Add modules" ), wxITEM_NORMAL );

    item->SetBitmap( module_xpm );
    placeMenu->Append( item );

    // Track
    text = AddHotkeyName( _( "Track" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ADD_NEW_TRACK, false );
    item = new wxMenuItem( placeMenu, ID_TRACK_BUTT, text,
                           _( "Add tracks and vias" ), wxITEM_NORMAL );

    item->SetBitmap( add_tracks_xpm );
    placeMenu->Append( item );

    // Zone
    item = new wxMenuItem( placeMenu, ID_PCB_ZONES_BUTT,
                         _( "Zone" ),
                         _( "Add filled zones" ));
    item->SetBitmap( add_zone_xpm );
    placeMenu->Append( item );

    // Text
    item = new wxMenuItem( placeMenu, ID_PCB_ADD_TEXT_BUTT,
                           _( "Text" ),
                           _( "Add text on copper layers or graphic text" ) );
    item->SetBitmap( add_text_xpm );
    placeMenu->Append( item );

    // Graphic Arc
    item = new wxMenuItem( placeMenu, ID_PCB_ARC_BUTT,
                         _( "Arc" ),
                         _( "Add graphic arc" ) );
    item->SetBitmap( add_arc_xpm );
    placeMenu->Append( item );

    // Graphic Circle
    item = new wxMenuItem( placeMenu, ID_PCB_CIRCLE_BUTT,
                         _( "Circle" ),
                         _( "Add graphic circle" ));
    item->SetBitmap( add_circle_xpm );
    placeMenu->Append( item );

    // Line or Polygon
    item = new wxMenuItem( placeMenu, ID_PCB_ADD_LINE_BUTT,
                           _( "Line or Polygon" ),
                           _( "Add graphic line or polygon" ));
    item->SetBitmap( add_dashed_line_xpm );
    placeMenu->Append( item );
    placeMenu->AppendSeparator();

    // Dimension
    item = new wxMenuItem( placeMenu, ID_PCB_DIMENSION_BUTT,
                         _( "Dimension" ),
                         _( "Add dimension" ) );
    item->SetBitmap( add_dimension_xpm );
    placeMenu->Append( item );

    // Layer alignment target
    item = new wxMenuItem( placeMenu, ID_PCB_MIRE_BUTT,
                           _( "Layer alignment target" ),
                           _( "Add layer alignment target" ));
    item->SetBitmap( add_mires_xpm );
    placeMenu->Append( item );
    placeMenu->AppendSeparator();

    // Drill & Place Offset
    item = new wxMenuItem( placeMenu, ID_PCB_PLACE_OFFSET_COORD_BUTT,
                      _( "Drill and Place Offset" ),
                      _( "Place the origin point for drill and place files" ));
    item->SetBitmap( pcb_offset_xpm );
    placeMenu->Append( item );

    // Grid Origin
    item = new wxMenuItem( placeMenu, ID_PCB_PLACE_GRID_COORD_BUTT,
                           _( "Grid Origin" ),
                           _( "Set the origin point for the grid" ));
    item->SetBitmap( grid_select_axis_xpm );
    placeMenu->Append( item );



    /** Create Preferences and configuration menu **/
    wxMenu* configmenu = new wxMenu;

    // Library
    item = new wxMenuItem( configmenu, ID_CONFIG_REQ,
                           _( "&Library" ),
                           _( "Setting libraries, directories and others..." ) );
    item->SetBitmap( library_xpm );
    configmenu->Append( item );

    // Colors and Visibility are also handled by the layers manager toolbar
    item = new wxMenuItem( configmenu, ID_MENU_PCB_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
                           _( "Hide &Layers Manager" ),
                           HELP_SHOW_HIDE_LAYERMANAGER );
    item->SetBitmap( layers_manager_xpm );
    configmenu->Append( item );

    // General
    item = new wxMenuItem( configmenu, ID_OPTIONS_SETUP,
                           _( "&General" ),
                           _( "Select general options for PCBnew" ) );
    item->SetBitmap( preference_xpm );
    configmenu->Append( item );

    //Display
    item = new wxMenuItem( configmenu, ID_PCB_DISPLAY_OPTIONS_SETUP,
                           _( "&Display" ),
                           _( "Select how items (pads, tracks texts ... ) are displayed" ) );
    item->SetBitmap( display_options_xpm );
    configmenu->Append( item );

    /* Create Dimensions submenu */
    wxMenu* dimensionsMenu = new wxMenu;

    // Grid
    item = new wxMenuItem( dimensionsMenu, ID_PCB_USER_GRID_SETUP,
                           _( "Grid" ),
                           _( "Adjust user grid dimensions" ) );
    item->SetBitmap( grid_xpm );
    dimensionsMenu->Append( item );

    // Text and Drawings
    item = new wxMenuItem( dimensionsMenu, ID_PCB_DRAWINGS_WIDTHS_SETUP,
                           _( "Texts and Drawings" ),
                           _( "Adjust dimensions for texts and drawings" ) );
    item->SetBitmap( options_text_xpm );
    dimensionsMenu->Append( item );

    // Pads
    item = new wxMenuItem( dimensionsMenu, ID_PCB_PAD_SETUP,
                           _( "Pads" ),
                           _( "Adjust default pad characteristics" ) );
    item->SetBitmap( pad_xpm );
    dimensionsMenu->Append( item );

    // Pads Mask Clearance
    item = new wxMenuItem( dimensionsMenu, ID_PCB_MASK_CLEARANCE,
                           _( "Pads Mask Clearance" ),
                           _( "Adjust the global clearance between pads and the solder resist mask" ) );
    item->SetBitmap( pads_mask_layers_xpm );
    dimensionsMenu->Append( item );


    // Save dimension preferences
    dimensionsMenu->AppendSeparator();
    item = new wxMenuItem( dimensionsMenu, ID_CONFIG_SAVE,
                           _( "&Save" ),
                           _( "Save dimension preferences" ) );
    item->SetBitmap( save_xpm );
    dimensionsMenu->Append( item );

    /* append dimension menu to config menu */
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( configmenu, dimensionsMenu,
                                        -1, _( "Di&mensions" ),
                                        _( "Global dimensions preferences" ),
                                        add_dimension_xpm );

    // Language submenu
    wxGetApp().AddMenuLanguageList( configmenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( configmenu );
    configmenu->AppendSeparator();

    // Save Preferences
    item = new wxMenuItem( configmenu, ID_CONFIG_SAVE,
                           _( "&Save Preferences" ),
                           _( "Save application preferences" ) );
    item->SetBitmap( save_setup_xpm );
    configmenu->Append( item );

    // Read Preferences
    item = new wxMenuItem( configmenu, ID_CONFIG_READ,
                           _( "&Read Preferences" ),
                           _( "Read application preferences" ) );
    item->SetBitmap( read_setup_xpm );
    configmenu->Append( item );



    /**
     * Design Rules menu
     */
    wxMenu* designRulesMenu = new wxMenu;

    /* Design Rules */
    item = new wxMenuItem( designRulesMenu, ID_MENU_PCB_SHOW_DESIGN_RULES_DIALOG,
                           _( "Design Rules" ),
                           _( "Open the design rules editor" ) );
    item->SetBitmap( hammer_xpm );
    designRulesMenu->Append( item );

    /* Layers Setup */
    item = new wxMenuItem( configmenu, ID_PCB_LAYERS_SETUP,
                           _( "&Layers Setup" ),
                           _( "Enable and set layer properties" ) );
    item->SetBitmap( copper_layers_setup_xpm );
    designRulesMenu->Append( item );


    /**
     * Help menu
     */
    wxMenu* helpMenu = new wxMenu;

    AddHelpVersionInfoMenuEntry( helpMenu );

    /* Contents */
    item = new wxMenuItem( helpMenu, ID_GENERAL_HELP,
                           _( "&Contents" ),
                           _( "Open the on line PCBnew documentation" ) );
    item->SetBitmap( online_help_xpm );
    helpMenu->Append( item );

    /* About */
#if !defined(__WXMAC__)

    item = new wxMenuItem( helpMenu, ID_KICAD_ABOUT,
                           _( "&About" ),
                           _( "About PCBnew printed circuit board designer" ) );
    item->SetBitmap( info_xpm );
    helpMenu->Append( item );

#endif /* !defined(__WXMAC__) */



    /**
     * Append all menus to the menuBar
     */
    menuBar->Append( filesMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( configmenu, _( "&Preferences" ) );
    menuBar->Append( designRulesMenu, _( "&Design Rules" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    /* Associate the menu bar with the frame */
    SetMenuBar( menuBar );
}
