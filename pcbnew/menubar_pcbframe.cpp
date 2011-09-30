/**
 * @file menubar_pcbframe.cpp
 * Pcbnew editor menu bar
 */
#include "fctsys.h"
#include "appl_wxstruct.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "protos.h"
#include "hotkeys.h"
#include "pcbnew_id.h"

#include "help_common_strings.h"

/**
 * Pcbnew mainframe menubar
 */
void PCB_EDIT_FRAME::ReCreateMenuBar()
{
    wxString    text;
    wxMenuItem* item;
    wxMenuBar*  menuBar = GetMenuBar();

    if( ! menuBar )
        menuBar = new wxMenuBar();

    // Delete all existing menus so they can be rebuilt.
    // This allows language changes of the menu text on the fly.
    menuBar->Freeze();

    while( menuBar->GetMenuCount() )
        delete menuBar->Remove(0);

    // Recreate all menus:

    // Create File Menu
    wxMenu* filesMenu = new wxMenu;

    // New
    item = new wxMenuItem( filesMenu, ID_NEW_BOARD,
                          _( "&New" ),
                          _( "Clear current board and initialize a new one" ) );
    SET_BITMAP( KiBitmap( new_xpm ) );
    filesMenu->Append( item );

    // Open
    text = AddHotkeyName( _( "&Open" ), g_Board_Editor_Hokeys_Descr,
                          HK_LOAD_BOARD );
    AddMenuItem( filesMenu, ID_LOAD_FILE, text,
                _( "Delete current board and load new board" ),
                KiBitmap( open_document_xpm ) );

    // Load Recent submenu
    static wxMenu* openRecentMenu;

    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentMenu )
        wxGetApp().m_fileHistory.RemoveMenu( openRecentMenu );

    openRecentMenu = new wxMenu();
    wxGetApp().m_fileHistory.UseMenu( openRecentMenu );
    wxGetApp().m_fileHistory.AddFilesToMenu();
    AddMenuItem( filesMenu, openRecentMenu,
                 -1, _( "Open &Recent" ),
                 _( "Open a recent opened board" ),
                 KiBitmap( open_project_xpm ) );


    // Pcbnew Board
    item = new wxMenuItem( filesMenu, ID_APPEND_FILE,
                           _( "&Append Board" ),
                           _( "Append another Pcbnew board to the current loaded board" ) );
    SET_BITMAP( KiBitmap( import_xpm ) );
    filesMenu->Append( item );

    // Separator
    filesMenu->AppendSeparator();

    // Save
    text = AddHotkeyName( _( "&Save" ), g_Board_Editor_Hokeys_Descr,
                          HK_SAVE_BOARD );
    AddMenuItem( filesMenu, ID_SAVE_BOARD, text,
                _( "Save current board" ),
                KiBitmap( save_xpm ) );

    // Save As
    item = new wxMenuItem( filesMenu, ID_SAVE_BOARD_AS,
                          _( "Save as..." ),
                          _( "Save the current board as.." ) );
    SET_BITMAP( KiBitmap( save_as_xpm ) );
    filesMenu->Append( item );
    filesMenu->AppendSeparator();

    // Revert
    item = new wxMenuItem( filesMenu, ID_MENU_READ_LAST_SAVED_VERSION_BOARD,
                           _( "&Revert" ),
                           _( "Clear board and get previous saved version of board" ) );
    SET_BITMAP( KiBitmap( jigsaw_xpm ) );
    filesMenu->Append( item );

    // Rescue
    item = new wxMenuItem( filesMenu, ID_MENU_RECOVER_BOARD, _( "&Rescue" ),
                           _( "Clear old board and get last rescue file" ) );
    SET_BITMAP( KiBitmap( hammer_xpm ) );
    filesMenu->Append( item );
    filesMenu->AppendSeparator();


    /* Fabrication Outputs submenu */
    wxMenu* fabricationOutputsMenu = new wxMenu;
    item = new wxMenuItem( fabricationOutputsMenu, ID_PCB_GEN_POS_MODULES_FILE,
                           _( "&Modules Position File" ),
                           _( "Generate modules position file for pick and place" ) );
    SET_BITMAP( KiBitmap( post_compo_xpm ) );
    fabricationOutputsMenu->Append( item );

    item = new wxMenuItem( fabricationOutputsMenu, ID_PCB_GEN_DRILL_FILE,
                           _( "&Drill File" ),
                           _( "Generate excellon2 drill file" ) );
    SET_BITMAP( KiBitmap( post_drill_xpm ) );
    fabricationOutputsMenu->Append( item );

    // Component File
    item = new wxMenuItem( fabricationOutputsMenu, ID_PCB_GEN_CMP_FILE,
                           _( "&Component File" ),
                           _( "(Re)create components file (*.cmp) for CvPcb" ) );
    SET_BITMAP( KiBitmap( create_cmp_file_xpm ) );
    fabricationOutputsMenu->Append( item );

    // BOM File
    item = new wxMenuItem( fabricationOutputsMenu, ID_PCB_GEN_BOM_FILE_FROM_BOARD,
                           _( "&BOM File" ),
                           _( "Create a bill of materials from schematic" ) );
    SET_BITMAP( KiBitmap( tools_xpm ) );
    fabricationOutputsMenu->Append( item );

    // Fabrications Outputs submenu append
    AddMenuItem( filesMenu, fabricationOutputsMenu,
                 -1, _( "Fabrication Outputs" ),
                 _( "Generate files for fabrication" ),
                 KiBitmap( fabrication_xpm ) );



    /** Import submenu **/
    wxMenu* submenuImport = new wxMenu();

    // Specctra Session
    item = new wxMenuItem( submenuImport, ID_GEN_IMPORT_SPECCTRA_SESSION,
                           _( "&Specctra Session" ),
                           _( "Import a routed \"Specctra Session\" (*.ses) file" ) );
    SET_BITMAP( KiBitmap( import_xpm ) );    // @todo need better bitmap
    submenuImport->Append( item );

    AddMenuItem( filesMenu, submenuImport,
                 ID_GEN_IMPORT_FILE, _( "Import" ),
                 _( "Import files" ), KiBitmap( import_xpm ) );



    /** Export submenu **/
    wxMenu* submenuexport = new wxMenu();

    // Specctra DSN
    item = new wxMenuItem( submenuexport, ID_GEN_EXPORT_SPECCTRA,
                           _( "&Specctra DSN" ),
                           _( "Export the current board to a \"Specctra DSN\" file" ) );
    SET_BITMAP( KiBitmap( export_xpm ) );
    submenuexport->Append( item );

    // GenCAD
    item = new wxMenuItem( submenuexport, ID_GEN_EXPORT_FILE_GENCADFORMAT,
                           _( "&GenCAD" ), _( "Export GenCAD format" ) );
    SET_BITMAP( KiBitmap( export_xpm ) );
    submenuexport->Append( item );

    // Module Report
    item = new wxMenuItem( submenuexport, ID_GEN_EXPORT_FILE_MODULE_REPORT,
                           _( "&Module Report" ),
                           _( "Create a report of all modules on the current board" ) );
    SET_BITMAP( KiBitmap( tools_xpm ) );
    submenuexport->Append( item );

    // VRML
    item = new wxMenuItem( submenuexport, ID_GEN_EXPORT_FILE_VRML,
                           _( "&VRML" ),
                           _( "Export a VRML board representation" ) );
    SET_BITMAP( KiBitmap( three_d_xpm ) );
    submenuexport->Append( item );

    AddMenuItem( filesMenu, submenuexport,
                 ID_GEN_EXPORT_FILE, _( "&Export" ),
                 _( "Export board" ), KiBitmap( export_xpm ) );

    filesMenu->AppendSeparator();

    // Page settings
    item = new wxMenuItem( filesMenu, ID_SHEET_SET,
                           _( "&Page settings" ),
                           _( "Page settings for paper size and texts" ) );
    SET_BITMAP( KiBitmap( sheetset_xpm ) );
    filesMenu->Append( item );

    // Print
    item = new wxMenuItem( filesMenu, wxID_PRINT,
                           _( "&Print" ),
                           _( "Print board" ) );
    SET_BITMAP( KiBitmap( print_button_xpm ) );
    filesMenu->Append( item );

    // Create SVG file
    item = new wxMenuItem( filesMenu, ID_GEN_PLOT_SVG,
                           _( "Print S&VG" ),
                           _( "Plot board in Scalable Vector Graphics format" ) );
    SET_BITMAP( KiBitmap( print_button_xpm ) );
    filesMenu->Append( item );

    // Plot
    item = new wxMenuItem( filesMenu, ID_GEN_PLOT,
              _( "&Plot" ),
              _( "Plot board in HPGL, PostScript or Gerber RS-274X format)" ) );
    SET_BITMAP( KiBitmap( plot_xpm ) );
    filesMenu->Append( item );
   filesMenu->AppendSeparator();

    wxMenu* submenuarchive = new wxMenu();

    // Archive New Footprints
    item = new wxMenuItem( submenuarchive, ID_MENU_ARCHIVE_NEW_MODULES,
                           _( "Archive New Footprints" ),
                           _( "Archive new footprints only in a library (keep other footprints in this lib)" ) );
    SET_BITMAP( KiBitmap( library_update_xpm ) );
    submenuarchive->Append( item );

    // Create FootPrint Archive
    item = new wxMenuItem( submenuarchive, ID_MENU_ARCHIVE_ALL_MODULES,
                           _( "Create Footprint Archive" ),
                           _( "Archive all footprints in a library (old library will be deleted)" ) );
    SET_BITMAP( KiBitmap( library_xpm ) );
    submenuarchive->Append( item );

    AddMenuItem( filesMenu, submenuarchive,
                 ID_MENU_ARCHIVE_MODULES,
                 _( "Archive Footprints" ),
                 _( "Archive or add footprints in a library file" ),
                 KiBitmap( library_xpm ) );

    /* Quit */
    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, wxID_EXIT, _( "&Quit" ), _( "Quit Pcbnew" ) );
    SET_BITMAP( KiBitmap( exit_xpm ) );
    filesMenu->Append( item );

    /** Create Edit menu **/
    wxMenu* editMenu = new wxMenu;

    // Undo
    text  = AddHotkeyName( _( "Undo" ), g_Pcbnew_Editor_Hokeys_Descr, HK_UNDO );
    item = new wxMenuItem( editMenu, wxID_UNDO, text,
                           HELP_UNDO, wxITEM_NORMAL );
    SET_BITMAP( KiBitmap( undo_xpm ) );
    editMenu->Append( item );

    // Redo
    text  = AddHotkeyName( _( "Redo" ), g_Pcbnew_Editor_Hokeys_Descr, HK_REDO );
    item = new wxMenuItem( editMenu, wxID_REDO, text,
                           HELP_REDO, wxITEM_NORMAL );
    SET_BITMAP( KiBitmap( redo_xpm ) );
    editMenu->Append( item );

    // Delete
    item = new wxMenuItem( editMenu, ID_PCB_DELETE_ITEM_BUTT,
                           _( "Delete" ),
                           _( "Delete items" ) );
    SET_BITMAP( KiBitmap( delete_body_xpm ) );
    editMenu->Append( item );
    editMenu->AppendSeparator();

    // Find
    text = AddHotkeyName( _( "&Find" ), g_Pcbnew_Editor_Hokeys_Descr, HK_FIND_ITEM );
    item = new wxMenuItem( editMenu, ID_FIND_ITEMS,
                           text, HELP_FIND );
    SET_BITMAP( KiBitmap( find_xpm ) );
    editMenu->Append( item );
    editMenu->AppendSeparator();

    // Global Deletions
    item = new wxMenuItem( editMenu, ID_PCB_GLOBAL_DELETE,
                           _( "Global &Deletions" ),
                           _( "Delete tracks, modules, texts... on board" ) );
    SET_BITMAP( KiBitmap( general_deletions_xpm ) );
    editMenu->Append( item );

    // Cleanup Tracks and Vias
    item = new wxMenuItem( editMenu, ID_MENU_PCB_CLEAN,
                           _( "&Cleanup Tracks and Vias" ),
                           _( "Clean stubs, vias, delete break points, or connect dangling tracks to pads and vias" ) );
    SET_BITMAP( KiBitmap( delete_body_xpm ) );
    editMenu->Append( item );

    // Swap Layers
    AddMenuItem( editMenu, ID_MENU_PCB_SWAP_LAYERS,
                 _( "&Swap Layers" ),
                 _( "Swap tracks on copper layers or drawings on other layers" ),
                 KiBitmap( swap_layer_xpm ) );

    // Reset module reference sizes
    item = new wxMenuItem( editMenu,
                           ID_MENU_PCB_RESET_TEXTMODULE_REFERENCE_SIZES,
                           _( "Reset Module &Reference Sizes" ),
                           _( "Reset text size and width of all module references to current defaults" ) );
    SET_BITMAP( KiBitmap( reset_text_xpm ) );
    editMenu->Append( item );

    // Reset module value sizes
    item = new wxMenuItem( editMenu,
                           ID_MENU_PCB_RESET_TEXTMODULE_VALUE_SIZES,
                           _( "Reset Module &Value Sizes" ),
                           _( "Reset text size and width of all module values to current defaults" ) );
    SET_BITMAP( KiBitmap( reset_text_xpm ) );
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
                          HK_ZOOM_IN, IS_ACCELERATOR );
    item = new wxMenuItem( viewMenu, ID_ZOOM_IN, text, HELP_ZOOM_IN, wxITEM_NORMAL );
    SET_BITMAP( KiBitmap( zoom_in_xpm ) );
    viewMenu->Append( item );

    // Zoom Out
    text = AddHotkeyName( _( "Zoom Out" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ZOOM_OUT, IS_ACCELERATOR );
    item = new wxMenuItem( viewMenu, ID_ZOOM_OUT, text, HELP_ZOOM_OUT, wxITEM_NORMAL );

    SET_BITMAP( KiBitmap( zoom_out_xpm ) );
    viewMenu->Append( item );

    // Fit on Screen
    text = AddHotkeyName( _( "Fit on Screen" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ZOOM_AUTO );

    item = new wxMenuItem( viewMenu, ID_ZOOM_PAGE, text, HELP_ZOOM_FIT, wxITEM_NORMAL );
    SET_BITMAP( KiBitmap( zoom_fit_in_page_xpm ) );
    viewMenu->Append( item );

    viewMenu->AppendSeparator();

    // Redraw
    text = AddHotkeyName( _( "Redraw" ), g_Pcbnew_Editor_Hokeys_Descr, HK_ZOOM_REDRAW );

    item = new wxMenuItem( viewMenu, ID_ZOOM_REDRAW, text,
                           HELP_ZOOM_REDRAW, wxITEM_NORMAL );
    SET_BITMAP( KiBitmap( zoom_redraw_xpm ) );
    viewMenu->Append( item );
    viewMenu->AppendSeparator();

    // 3D Display
    item = new wxMenuItem( viewMenu, ID_MENU_PCB_SHOW_3D_FRAME,
                           _( "3D Display" ),
                           _( "Show board in 3D viewer" ) );
    SET_BITMAP( KiBitmap( three_d_xpm ) );
    viewMenu->Append( item );

    // List Nets
    item = new wxMenuItem( viewMenu, ID_MENU_LIST_NETS,
                           _( "&List Nets" ),
                           _( "View a list of nets with names and id's" ) );
    SET_BITMAP( KiBitmap( tools_xpm ) );
    viewMenu->Append( item );



    /** Create Place Menu **/
    wxMenu* placeMenu = new wxMenu;

    // Module
    text = AddHotkeyName( _( "Module" ), g_Pcbnew_Editor_Hokeys_Descr, HK_ADD_MODULE, IS_ACCELERATOR );
    item = new wxMenuItem( placeMenu, ID_PCB_MODULE_BUTT, text,
                           _( "Add modules" ), wxITEM_NORMAL );

    SET_BITMAP( KiBitmap( module_xpm ) );
    placeMenu->Append( item );

    // Track
    text = AddHotkeyName( _( "Track" ), g_Pcbnew_Editor_Hokeys_Descr, HK_ADD_NEW_TRACK, IS_ACCELERATOR );
    item = new wxMenuItem( placeMenu, ID_TRACK_BUTT, text,
                           _( "Add tracks and vias" ), wxITEM_NORMAL );

    SET_BITMAP( KiBitmap( add_tracks_xpm ) );
    placeMenu->Append( item );

    // Zone
    item = new wxMenuItem( placeMenu, ID_PCB_ZONES_BUTT,
                         _( "Zone" ),
                         _( "Add filled zones" ));
    SET_BITMAP( KiBitmap( add_zone_xpm ) );
    placeMenu->Append( item );

    // Text
    item = new wxMenuItem( placeMenu, ID_PCB_ADD_TEXT_BUTT,
                           _( "Text" ),
                           _( "Add text on copper layers or graphic text" ) );
    SET_BITMAP( KiBitmap( add_text_xpm ) );
    placeMenu->Append( item );

    // Graphic Arc
    item = new wxMenuItem( placeMenu, ID_PCB_ARC_BUTT,
                         _( "Arc" ),
                         _( "Add graphic arc" ) );
    SET_BITMAP( KiBitmap( add_arc_xpm ) );
    placeMenu->Append( item );

    // Graphic Circle
    item = new wxMenuItem( placeMenu, ID_PCB_CIRCLE_BUTT,
                         _( "Circle" ),
                         _( "Add graphic circle" ));
    SET_BITMAP( KiBitmap( add_circle_xpm ) );
    placeMenu->Append( item );

    // Line or Polygon
    item = new wxMenuItem( placeMenu, ID_PCB_ADD_LINE_BUTT,
                           _( "Line or Polygon" ),
                           _( "Add graphic line or polygon" ));
    SET_BITMAP( KiBitmap( add_dashed_line_xpm ) );
    placeMenu->Append( item );
    placeMenu->AppendSeparator();

    // Dimension
    item = new wxMenuItem( placeMenu, ID_PCB_DIMENSION_BUTT,
                           _( "Dimension" ),
                           _( "Add dimension" ) );
    SET_BITMAP( KiBitmap( add_dimension_xpm ) );
    placeMenu->Append( item );

    // Layer alignment target
    item = new wxMenuItem( placeMenu, ID_PCB_MIRE_BUTT,
                           _( "Layer alignment target" ),
                           _( "Add layer alignment target" ));
    SET_BITMAP( KiBitmap( add_mires_xpm ) );
    placeMenu->Append( item );
    placeMenu->AppendSeparator();

    // Drill & Place Offset
    item = new wxMenuItem( placeMenu, ID_PCB_PLACE_OFFSET_COORD_BUTT,
                           _( "Drill and Place Offset" ),
                           _( "Place the origin point for drill and place files" ));
    SET_BITMAP( KiBitmap( pcb_offset_xpm ) );
    placeMenu->Append( item );

    // Grid Origin
    item = new wxMenuItem( placeMenu, ID_PCB_PLACE_GRID_COORD_BUTT,
                           _( "Grid Origin" ),
                           _( "Set the origin point for the grid" ));
    SET_BITMAP( KiBitmap( grid_select_axis_xpm ) );
    placeMenu->Append( item );



    /** Create Preferences and configuration menu **/
    wxMenu* configmenu = new wxMenu;

    // Library
    item = new wxMenuItem( configmenu, ID_CONFIG_REQ,
                           _( "&Library" ),
                           _( "Setting libraries, directories and others..." ) );
    SET_BITMAP( KiBitmap( library_xpm ) );
    configmenu->Append( item );

    // Colors and Visibility are also handled by the layers manager toolbar
    item = new wxMenuItem( configmenu, ID_MENU_PCB_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
                           m_show_layer_manager_tools ?
                           _( "Hide &Layers Manager" ) : _("Show &Layers Manager" ),
                           HELP_SHOW_HIDE_LAYERMANAGER );
    SET_BITMAP( KiBitmap( layers_manager_xpm ) );
    configmenu->Append( item );

    // General
#ifdef __WXMAC__
    configmenu->Append(wxID_PREFERENCES);
#else
    item = new wxMenuItem( configmenu, wxID_PREFERENCES,
                           _( "&General" ),
                           _( "Select general options for Pcbnew" ) );

    SET_BITMAP( KiBitmap( preference_xpm ) );
    configmenu->Append( item );
#endif

    // Display
    item = new wxMenuItem( configmenu, ID_PCB_DISPLAY_OPTIONS_SETUP,
                           _( "&Display" ),
                           _( "Select how items (pads, tracks texts ... ) are displayed" ) );
    SET_BITMAP( KiBitmap( display_options_xpm ) );
    configmenu->Append( item );

    // Create Dimensions submenu
    wxMenu* dimensionsMenu = new wxMenu;

    // Grid
    item = new wxMenuItem( dimensionsMenu, ID_PCB_USER_GRID_SETUP,
                           _( "Grid" ),
                           _( "Adjust user grid dimensions" ) );
    SET_BITMAP( KiBitmap( grid_xpm ) );
    dimensionsMenu->Append( item );

    // Text and Drawings
    item = new wxMenuItem( dimensionsMenu, ID_PCB_DRAWINGS_WIDTHS_SETUP,
                           _( "Texts and Drawings" ),
                           _( "Adjust dimensions for texts and drawings" ) );
    SET_BITMAP( KiBitmap( options_text_xpm ) );
    dimensionsMenu->Append( item );

    // Pads
    item = new wxMenuItem( dimensionsMenu, ID_PCB_PAD_SETUP,
                           _( "Pads" ),
                           _( "Adjust default pad characteristics" ) );
    SET_BITMAP( KiBitmap( pad_xpm ) );
    dimensionsMenu->Append( item );

    // Pads Mask Clearance
    item = new wxMenuItem( dimensionsMenu, ID_PCB_MASK_CLEARANCE,
                           _( "Pads Mask Clearance" ),
                           _( "Adjust the global clearance between pads and the solder resist mask" ) );
    SET_BITMAP( KiBitmap( pads_mask_layers_xpm ) );
    dimensionsMenu->Append( item );


    // Save dimension preferences
    dimensionsMenu->AppendSeparator();
    item = new wxMenuItem( dimensionsMenu, ID_CONFIG_SAVE,
                           _( "&Save" ),
                           _( "Save dimension preferences" ) );
    SET_BITMAP( KiBitmap( save_xpm ) );
    dimensionsMenu->Append( item );

    // Append dimension menu to config menu
    AddMenuItem( configmenu, dimensionsMenu,
                 -1, _( "Di&mensions" ),
                 _( "Global dimensions preferences" ),
                 KiBitmap( add_dimension_xpm ) );

    // Language submenu
    wxGetApp().AddMenuLanguageList( configmenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( configmenu );


    // Macros submenu
    wxMenu* macrosMenu = new wxMenu;

    item = new wxMenuItem( macrosMenu, ID_PREFRENCES_MACROS_SAVE,
                           _( "Save macros" ),
                           _( "Save macros to file" ) );
    macrosMenu->Append( item );

    item = new wxMenuItem( macrosMenu, ID_PREFRENCES_MACROS_READ,
                           _( "Read macros" ),
                           _( "Read macros from file" ) );
    macrosMenu->Append( item );

    // Append macros menu to config menu
    AddMenuItem( configmenu, macrosMenu,
                 -1, _( "Macros" ),
                 _( "Macros save/read operations" ),
                 KiBitmap( add_dimension_xpm ) );

    configmenu->AppendSeparator();

    // Save Preferences
    item = new wxMenuItem( configmenu, ID_CONFIG_SAVE,
                           _( "&Save Preferences" ),
                           _( "Save application preferences" ) );
    SET_BITMAP( KiBitmap( save_setup_xpm ) );
    configmenu->Append( item );

    // Read Preferences
    item = new wxMenuItem( configmenu, ID_CONFIG_READ,
                           _( "&Read Preferences" ),
                           _( "Read application preferences" ) );
    SET_BITMAP( KiBitmap( read_setup_xpm ) );
    configmenu->Append( item );

    /**
     * Tools menu
     */
    wxMenu* toolsMenu = new wxMenu;

    /* Netlist */
    item = new wxMenuItem( toolsMenu, ID_GET_NETLIST,
                           _( "Netlist" ),
                           _( "Read the netlist and update board connectivity" ) );
    SET_BITMAP( KiBitmap( netlist_xpm ) );
    toolsMenu->Append( item );

    /* Layer pair */
    AddMenuItem( toolsMenu, ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR,
                           _( "Layer Pair" ),
                           _( "Change the active layer pair" ),
                           KiBitmap( select_layer_pair_xpm ) );

    /* DRC */
    item = new wxMenuItem( toolsMenu, ID_DRC_CONTROL,
                           _( "DRC" ),
                           _( "Perform design rules check" ) );
    SET_BITMAP( KiBitmap( erc_xpm ) );
    toolsMenu->Append( item );

    /* FreeRoute */
    item = new wxMenuItem( toolsMenu, ID_TOOLBARH_PCB_FREEROUTE_ACCESS,
                           _( "FreeRoute" ),
                           _( "Fast access to the Web Based FreeROUTE advanced router" ) );
    SET_BITMAP( KiBitmap( web_support_xpm ) );
    toolsMenu->Append( item );

    /**
     * Design Rules menu
     */
    wxMenu* designRulesMenu = new wxMenu;

    // Design Rules
    item = new wxMenuItem( designRulesMenu, ID_MENU_PCB_SHOW_DESIGN_RULES_DIALOG,
                           _( "Design Rules" ),
                           _( "Open the design rules editor" ) );
    SET_BITMAP( KiBitmap( hammer_xpm ) );
    designRulesMenu->Append( item );

    // Layers Setup
    item = new wxMenuItem( configmenu, ID_PCB_LAYERS_SETUP,
                           _( "&Layers Setup" ),
                           _( "Enable and set layer properties" ) );
    SET_BITMAP( KiBitmap( copper_layers_setup_xpm ) );
    designRulesMenu->Append( item );

    /**
     * Help menu
     */
    wxMenu* helpMenu = new wxMenu;

    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contents
    AddMenuItem( helpMenu, wxID_HELP,
                 _( "&Contents" ),
                 _( "Open the pcbnew handbook" ),
                 KiBitmap( online_help_xpm ) );
    AddMenuItem( helpMenu, wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open the \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    // About
    helpMenu->AppendSeparator();
    AddMenuItem( helpMenu, wxID_ABOUT,
                 _( "&About Pcbnew" ),
                 _( "About Pcbnew printed circuit board designer" ),
                 KiBitmap( info_xpm ) );

    /**
     * Append all menus to the menuBar
     */
    menuBar->Append( filesMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( configmenu, _( "&Preferences" ) );
    menuBar->Append( toolsMenu, _( "&Tools" ) );
    menuBar->Append( designRulesMenu, _( "&Design Rules" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}
