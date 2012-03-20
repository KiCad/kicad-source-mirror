/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2010-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file menubar_pcbframe.cpp
 * Pcbnew editor menu bar
 */
#include <fctsys.h>
#include <appl_wxstruct.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <protos.h>
#include <hotkeys.h>
#include <pcbnew_id.h>

#include <help_common_strings.h>

/**
 * Pcbnew mainframe menubar
 */
void PCB_EDIT_FRAME::ReCreateMenuBar()
{
    wxString    text;
    wxMenuBar*  menuBar = GetMenuBar();

    if( ! menuBar )
        menuBar = new wxMenuBar();

    // Delete all existing menus so they can be rebuilt.
    // This allows language changes of the menu text on the fly.
    menuBar->Freeze();

    while( menuBar->GetMenuCount() )
        delete menuBar->Remove( 0 );

    // Recreate all menus:

    // Create File Menu
    wxMenu* filesMenu = new wxMenu;

    // New
    AddMenuItem( filesMenu, ID_NEW_BOARD,
                 _( "&New" ),
                 _( "Clear current board and initialize a new one" ),
                 KiBitmap( new_pcb_xpm ) );

    // Open
    text = AddHotkeyName( _( "&Open" ), g_Board_Editor_Hokeys_Descr, HK_LOAD_BOARD );
    AddMenuItem( filesMenu, ID_LOAD_FILE, text,
                 _( "Delete current board and load new board" ),
                 KiBitmap( open_brd_file_xpm ) );

    // Load Recent submenu
    static wxMenu* openRecentMenu;

    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentMenu )
        wxGetApp().GetFileHistory().RemoveMenu( openRecentMenu );

    openRecentMenu = new wxMenu();
    wxGetApp().GetFileHistory().UseMenu( openRecentMenu );
    wxGetApp().GetFileHistory().AddFilesToMenu();
    AddMenuItem( filesMenu, openRecentMenu,
                 -1, _( "Open &Recent" ),
                 _( "Open a recent opened board" ),
                 KiBitmap( open_project_xpm ) );


    // Pcbnew Board
    AddMenuItem( filesMenu, ID_APPEND_FILE,
                 _( "&Append Board" ),
                 _( "Append another Pcbnew board to the current loaded board" ),
                 KiBitmap( import_xpm ) );

    // Separator
    filesMenu->AppendSeparator();

    // Save
    text = AddHotkeyName( _( "&Save" ), g_Board_Editor_Hokeys_Descr, HK_SAVE_BOARD );
    AddMenuItem( filesMenu, ID_SAVE_BOARD, text,
                 _( "Save current board" ),
                 KiBitmap( save_xpm ) );

    // Save As
    AddMenuItem( filesMenu, ID_SAVE_BOARD_AS,
                 _( "Sa&ve As..." ),
                 _( "Save the current board as.." ),
                 KiBitmap( save_as_xpm ) );
    filesMenu->AppendSeparator();

    // Revert
    AddMenuItem( filesMenu, ID_MENU_READ_LAST_SAVED_VERSION_BOARD,
                 _( "Revert" ),
                 _( "Clear board and get previous saved version of board" ),
                 KiBitmap( jigsaw_xpm ) );

    // Rescue
    AddMenuItem( filesMenu, ID_MENU_RECOVER_BOARD, _( "Rescue" ),
                 _( "Clear old board and get last rescue file" ),
                 KiBitmap( hammer_xpm ) );
    filesMenu->AppendSeparator();


    /* Fabrication Outputs submenu */
    wxMenu* fabricationOutputsMenu = new wxMenu;
    AddMenuItem( fabricationOutputsMenu, ID_PCB_GEN_POS_MODULES_FILE,
                 _( "&Modules Position (.pos) File" ),
                 _( "Generate modules position file for pick and place" ),
                 KiBitmap( post_compo_xpm ) );

    AddMenuItem( fabricationOutputsMenu, ID_PCB_GEN_DRILL_FILE,
                 _( "&Drill (.drl) File" ),
                 _( "Generate excellon2 drill file" ),
                 KiBitmap( post_drill_xpm ) );

    // Module Report
    AddMenuItem( fabricationOutputsMenu, ID_GEN_EXPORT_FILE_MODULE_REPORT,
                 _( "&Module (.rpt) Report" ),
                 _( "Create a report of all modules on the current board" ),
                 KiBitmap( tools_xpm ) );

    // Component File
    AddMenuItem( fabricationOutputsMenu, ID_PCB_GEN_CMP_FILE,
                 _( "&Component (.cmp) File" ),
                 _( "(Re)create components file (*.cmp) for CvPcb" ),
                 KiBitmap( create_cmp_file_xpm ) );

    // BOM File
    AddMenuItem( fabricationOutputsMenu, ID_PCB_GEN_BOM_FILE_FROM_BOARD,
                 _( "&BOM File" ),
                 _( "Create a bill of materials from schematic" ),
                 KiBitmap( bom_xpm ) );

    // Fabrications Outputs submenu append
    AddMenuItem( filesMenu, fabricationOutputsMenu,
                 -1, _( "&Fabrication Outputs" ),
                 _( "Generate files for fabrication" ),
                 KiBitmap( fabrication_xpm ) );


    /** Import submenu **/
    wxMenu* submenuImport = new wxMenu();

    // Specctra Session
    AddMenuItem( submenuImport, ID_GEN_IMPORT_SPECCTRA_SESSION,
                           _( "&Specctra Session" ),
                           _( "Import a routed \"Specctra Session\" (*.ses) file" ),
                           KiBitmap( import_xpm ) );

    AddMenuItem( filesMenu, submenuImport,
                 ID_GEN_IMPORT_FILE, _( "&Import" ),
                 _( "Import files" ), KiBitmap( import_xpm ) );


    /** Export submenu **/
    wxMenu* submenuexport = new wxMenu();

    // Specctra DSN
    AddMenuItem( submenuexport, ID_GEN_EXPORT_SPECCTRA,
                 _( "&Specctra DSN" ),
                 _( "Export the current board to a \"Specctra DSN\" file" ),
                 KiBitmap( export_xpm ) );

    // GenCAD
    AddMenuItem( submenuexport, ID_GEN_EXPORT_FILE_GENCADFORMAT,
                 _( "&GenCAD" ), _( "Export GenCAD format" ),
                 KiBitmap( export_xpm ) );

    // VRML
    AddMenuItem( submenuexport, ID_GEN_EXPORT_FILE_VRML,
                 _( "&VRML" ),
                 _( "Export a VRML board representation" ),
                 KiBitmap( three_d_xpm ) );

    AddMenuItem( filesMenu, submenuexport,
                 ID_GEN_EXPORT_FILE, _( "E&xport" ),
                 _( "Export board" ), KiBitmap( export_xpm ) );

    filesMenu->AppendSeparator();

    // Page settings
    AddMenuItem( filesMenu, ID_SHEET_SET,
                 _( "Page s&ettings" ),
                 _( "Page settings for paper size and texts" ),
                 KiBitmap( sheetset_xpm ) );

    // Print
    AddMenuItem( filesMenu, wxID_PRINT,
                 _( "&Print" ), _( "Print board" ),
                 KiBitmap( print_button_xpm ) );

    // Create SVG file
    AddMenuItem( filesMenu, ID_GEN_PLOT_SVG,
                 _( "Print SV&G" ),
                 _( "Plot board in Scalable Vector Graphics format" ),
                 KiBitmap( print_button_xpm ) );

    // Plot
    AddMenuItem( filesMenu, ID_GEN_PLOT,
                 _( "P&lot" ),
                 _( "Plot board in HPGL, PostScript or Gerber RS-274X format)" ),
                 KiBitmap( plot_xpm ) );

    filesMenu->AppendSeparator();

    wxMenu* submenuarchive = new wxMenu();

    // Archive New Footprints
    AddMenuItem( submenuarchive, ID_MENU_ARCHIVE_NEW_MODULES,
                 _( "&Archive New Footprints" ),
                 _( "Archive new footprints only in a library (keep other footprints in this lib)" ),
                 KiBitmap( library_update_xpm ) );

    // Create FootPrint Archive
    AddMenuItem( submenuarchive, ID_MENU_ARCHIVE_ALL_MODULES,
                 _( "&Create Footprint Archive" ),
                 _( "Archive all footprints in a library (old library will be deleted)" ),
                 KiBitmap( library_xpm ) );

    AddMenuItem( filesMenu, submenuarchive,
                 ID_MENU_ARCHIVE_MODULES,
                 _( "Arc&hive Footprints" ),
                 _( "Archive or add footprints in a library file" ),
                 KiBitmap( library_xpm ) );

    /* Quit */
    filesMenu->AppendSeparator();
    AddMenuItem( filesMenu, wxID_EXIT, _( "&Quit" ), _( "Quit Pcbnew" ),
                 KiBitmap( exit_xpm ) );

    /** Create Edit menu **/
    wxMenu* editMenu = new wxMenu;

    // Undo
    text  = AddHotkeyName( _( "&Undo" ), g_Pcbnew_Editor_Hokeys_Descr, HK_UNDO );
    AddMenuItem( editMenu, wxID_UNDO, text, HELP_UNDO, KiBitmap( undo_xpm ) );

    // Redo
    text  = AddHotkeyName( _( "&Redo" ), g_Pcbnew_Editor_Hokeys_Descr, HK_REDO );
    AddMenuItem( editMenu, wxID_REDO, text, HELP_REDO, KiBitmap( redo_xpm ) );

    // Delete
    AddMenuItem( editMenu, ID_PCB_DELETE_ITEM_BUTT,
                 _( "&Delete" ), _( "Delete items" ),
                 KiBitmap( delete_xpm ) );

    editMenu->AppendSeparator();

    // Find
    text = AddHotkeyName( _( "&Find" ), g_Pcbnew_Editor_Hokeys_Descr, HK_FIND_ITEM );
    AddMenuItem( editMenu, ID_FIND_ITEMS, text, HELP_FIND , KiBitmap( find_xpm ) );

    editMenu->AppendSeparator();

    // Global Deletions
    AddMenuItem( editMenu, ID_PCB_GLOBAL_DELETE,
                 _( "&Global Deletions" ),
                 _( "Delete tracks, modules, texts... on board" ),
                 KiBitmap( general_deletions_xpm ) );

    // Cleanup Tracks and Vias
    AddMenuItem( editMenu, ID_MENU_PCB_CLEAN,
                 _( "&Cleanup Tracks and Vias" ),
                 _( "Clean stubs, vias, delete break points, or connect dangling tracks to pads and vias" ),
                 KiBitmap( delete_xpm ) );

    // Swap Layers
    AddMenuItem( editMenu, ID_MENU_PCB_SWAP_LAYERS,
                 _( "&Swap Layers" ),
                 _( "Swap tracks on copper layers or drawings on other layers" ),
                 KiBitmap( swap_layer_xpm ) );

    // Reset module reference sizes
    AddMenuItem( editMenu, ID_MENU_PCB_RESET_TEXTMODULE_REFERENCE_SIZES,
                 _( "Reset Module &Reference Sizes" ),
                 _( "Reset text size and width of all module references to current defaults" ),
                 KiBitmap( reset_text_xpm ) );

    // Reset module value sizes
    AddMenuItem( editMenu, ID_MENU_PCB_RESET_TEXTMODULE_VALUE_SIZES,
                 _( "Reset Module &Value Sizes" ),
                 _( "Reset text size and width of all module values to current defaults" ),
                 KiBitmap( reset_text_xpm ) );

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
    text = AddHotkeyName( _( "Zoom &In" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ZOOM_IN, IS_ACCELERATOR );
    AddMenuItem( viewMenu, ID_ZOOM_IN, text, HELP_ZOOM_IN, KiBitmap( zoom_in_xpm ) );

    // Zoom Out
    text = AddHotkeyName( _( "Zoom &Out" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ZOOM_OUT, IS_ACCELERATOR );
    AddMenuItem( viewMenu, ID_ZOOM_OUT, text, HELP_ZOOM_OUT, KiBitmap( zoom_out_xpm ) );

    // Fit on Screen
    text = AddHotkeyName( _( "&Fit on Screen" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ZOOM_AUTO );

    AddMenuItem( viewMenu, ID_ZOOM_PAGE, text, HELP_ZOOM_FIT,
                 KiBitmap( zoom_fit_in_page_xpm ) );

    viewMenu->AppendSeparator();

    // Redraw
    text = AddHotkeyName( _( "&Redraw" ), g_Pcbnew_Editor_Hokeys_Descr, HK_ZOOM_REDRAW );

    AddMenuItem( viewMenu, ID_ZOOM_REDRAW, text,
                 HELP_ZOOM_REDRAW, KiBitmap( zoom_redraw_xpm ) );
    viewMenu->AppendSeparator();

    // 3D Display
    AddMenuItem( viewMenu, ID_MENU_PCB_SHOW_3D_FRAME,
                 _( "3&D Display" ),_( "Show board in 3D viewer" ),
                 KiBitmap( three_d_xpm ) );

    // List Nets
    AddMenuItem( viewMenu, ID_MENU_LIST_NETS,
                 _( "&List Nets" ), _( "View a list of nets with names and id's" ),
                 KiBitmap( tools_xpm ) );

    /** Create Place Menu **/
    wxMenu* placeMenu = new wxMenu;

    // Module
    text = AddHotkeyName( _( "&Module" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ADD_MODULE, IS_ACCELERATOR );
    AddMenuItem( placeMenu, ID_PCB_MODULE_BUTT, text,
                 _( "Add modules" ), KiBitmap( module_xpm ) );

    // Track
    text = AddHotkeyName( _( "&Track" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ADD_NEW_TRACK, IS_ACCELERATOR );
    AddMenuItem( placeMenu, ID_TRACK_BUTT, text,
                 _( "Add tracks and vias" ), KiBitmap( add_tracks_xpm ) );

    // Zone
    AddMenuItem( placeMenu, ID_PCB_ZONES_BUTT,
                 _( "&Zone" ), _( "Add filled zones" ), KiBitmap( add_zone_xpm ) );

    // Text
    AddMenuItem( placeMenu, ID_PCB_ADD_TEXT_BUTT,
                  _( "Te&xt" ), _( "Add text on copper layers or graphic text" ),
                  KiBitmap( add_text_xpm ) );

    // Graphic Arc
    AddMenuItem( placeMenu, ID_PCB_ARC_BUTT,
                 _( "&Arc" ), _( "Add graphic arc" ),KiBitmap( add_arc_xpm ) );

    // Graphic Circle
    AddMenuItem( placeMenu, ID_PCB_CIRCLE_BUTT,
                 _( "&Circle" ), _( "Add graphic circle" ),
                 KiBitmap( add_circle_xpm ) );

    // Line or Polygon
    AddMenuItem( placeMenu, ID_PCB_ADD_LINE_BUTT,
                 _( "&Line or Polygon" ),
                 _( "Add graphic line or polygon" ),
                 KiBitmap( add_dashed_line_xpm ) );

    placeMenu->AppendSeparator();

    // Dimension
    AddMenuItem( placeMenu, ID_PCB_DIMENSION_BUTT,
                 _( "&Dimension" ), _( "Add dimension" ),
                 KiBitmap( add_dimension_xpm ) );

    // Layer alignment target
    AddMenuItem( placeMenu, ID_PCB_MIRE_BUTT,
                 _( "La&yer alignment target" ), _( "Add layer alignment target" ),
                 KiBitmap( add_mires_xpm ) );

    placeMenu->AppendSeparator();

    // Drill & Place Offset
    AddMenuItem( placeMenu, ID_PCB_PLACE_OFFSET_COORD_BUTT,
                 _( "Drill and Place O&ffset" ),
                 _( "Place the origin point for drill and place files" ),
                 KiBitmap( pcb_offset_xpm ) );

    // Grid Origin
    AddMenuItem( placeMenu, ID_PCB_PLACE_GRID_COORD_BUTT,
                 _( "&Grid Origin" ),
                 _( "Set the origin point for the grid" ),
                 KiBitmap( grid_select_axis_xpm ) );

    /* Create Preferences and configuration menu */
    wxMenu* configmenu = new wxMenu;

    // Library
    AddMenuItem( configmenu, ID_CONFIG_REQ,
                 _( "Li&brary" ), _( "Setting libraries, directories and others..." ),
                 KiBitmap( library_xpm ) );

    // Colors and Visibility are also handled by the layers manager toolbar
    AddMenuItem( configmenu, ID_MENU_PCB_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
                 m_show_layer_manager_tools ?
                 _( "Hide La&yers Manager" ) : _("Show La&yers Manager" ),
                 HELP_SHOW_HIDE_LAYERMANAGER,
                 KiBitmap( layers_manager_xpm ) );

    // General
#ifdef __WXMAC__
    configmenu->Append(wxID_PREFERENCES);
#else
    AddMenuItem( configmenu, wxID_PREFERENCES,
                 _( "&General" ), _( "Select general options for Pcbnew" ),
                 KiBitmap( preference_xpm ) );
#endif

    // Display
    AddMenuItem( configmenu, ID_PCB_DISPLAY_OPTIONS_SETUP,
                 _( "&Display" ),
                 _( "Select how items (pads, tracks texts ... ) are displayed" ),
                 KiBitmap( display_options_xpm ) );

    // Create Dimensions submenu
    wxMenu* dimensionsMenu = new wxMenu;

    // Grid
    AddMenuItem( dimensionsMenu, ID_PCB_USER_GRID_SETUP,
                 _( "G&rid" ),_( "Adjust user grid dimensions" ),
                 KiBitmap( grid_xpm ) );

    // Text and Drawings
    AddMenuItem( dimensionsMenu, ID_PCB_DRAWINGS_WIDTHS_SETUP,
                 _( "Te&xts and Drawings" ),
                 _( "Adjust dimensions for texts and drawings" ),
                 KiBitmap( options_text_xpm ) );

    // Pads
    AddMenuItem( dimensionsMenu, ID_PCB_PAD_SETUP,
                 _( "&Pads" ),  _( "Adjust default pad characteristics" ),
                 KiBitmap( pad_xpm ) );

    // Pads Mask Clearance
    AddMenuItem( dimensionsMenu, ID_PCB_MASK_CLEARANCE,
                 _( "Pads &Mask Clearance" ),
                 _( "Adjust the global clearance between pads and the solder resist mask" ),
                 KiBitmap( pads_mask_layers_xpm ) );

    // Save dimension preferences
    dimensionsMenu->AppendSeparator();
    AddMenuItem( dimensionsMenu, ID_CONFIG_SAVE,
                 _( "&Save" ), _( "Save dimension preferences" ),
                 KiBitmap( save_xpm ) );

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

    AddMenuItem( macrosMenu, ID_PREFRENCES_MACROS_SAVE,
                 _( "&Save macros" ),
                 _( "Save macros to file" ),
                 KiBitmap( save_setup_xpm ) );

    AddMenuItem( macrosMenu, ID_PREFRENCES_MACROS_READ,
                 _( "&Read macros" ),
                 _( "Read macros from file" ),
                 KiBitmap( read_setup_xpm ) );

    // Append macros menu to config menu
    AddMenuItem( configmenu, macrosMenu,
                 -1, _( "Ma&cros" ),
                 _( "Macros save/read operations" ),
                 KiBitmap( macros_record_xpm ) );

    configmenu->AppendSeparator();

    // Save Preferences
    AddMenuItem( configmenu, ID_CONFIG_SAVE,
                 _( "&Save Preferences" ),
                 _( "Save application preferences" ),
                 KiBitmap( save_setup_xpm ) );

    // Read Preferences
    AddMenuItem( configmenu, ID_CONFIG_READ,
                 _( "&Read Preferences" ),
                 _( "Read application preferences" ),
                 KiBitmap( read_setup_xpm ) );

    /**
     * Tools menu
     */
    wxMenu* toolsMenu = new wxMenu;

    /* Netlist */
    AddMenuItem( toolsMenu, ID_GET_NETLIST,
                 _( "&Netlist" ),
                 _( "Read the netlist and update board connectivity" ),
                 KiBitmap( netlist_xpm ) );

    /* Layer pair */
    AddMenuItem( toolsMenu, ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR,
                 _( "&Layer Pair" ), _( "Change the active layer pair" ),
                 KiBitmap( select_layer_pair_xpm ) );

    /* DRC */
    AddMenuItem( toolsMenu, ID_DRC_CONTROL,
                 _( "&DRC" ),
                 _( "Perform design rules check" ), KiBitmap( erc_xpm ) );

    /* FreeRoute */
    AddMenuItem( toolsMenu, ID_TOOLBARH_PCB_FREEROUTE_ACCESS,
                 _( "&FreeRoute" ),
                 _( "Fast access to the Web Based FreeROUTE advanced router" ),
                 KiBitmap( web_support_xpm ) );

    /* Design Rules menu
     */
    wxMenu* designRulesMenu = new wxMenu;

    // Design Rules
    AddMenuItem( designRulesMenu, ID_MENU_PCB_SHOW_DESIGN_RULES_DIALOG,
                 _( "Design Rules" ),
                 _( "Open the design rules editor" ), KiBitmap( hammer_xpm ) );

    // Layers Setup
    AddMenuItem( designRulesMenu, ID_PCB_LAYERS_SETUP,
                 _( "&Layers Setup" ),  _( "Enable and set layer properties" ),
                 KiBitmap( copper_layers_setup_xpm ) );

    /**
     * Help menu
     */
    wxMenu* helpMenu = new wxMenu;

    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contents
    AddMenuItem( helpMenu, wxID_HELP,
                 _( "&Contents" ),
                 _( "Open the Pcbnew handbook" ),
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
    menuBar->Append( configmenu, _( "P&references" ) );
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
