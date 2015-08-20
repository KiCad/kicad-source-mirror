/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pgm_base.h>
#include <kiface_i.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <hotkeys.h>
#include <pcbnew_id.h>

#include <help_common_strings.h>
#include <menus_helpers.h>

void PCB_EDIT_FRAME::ReCreateMenuBar()
{
    wxString    text;
    wxMenuBar*  menuBar = GetMenuBar();
    wxMenuItem * menutitem;

    wxFileHistory&  fhist = Kiface().GetFileHistory();

    if( !menuBar )
        menuBar = new wxMenuBar();

    // Delete all existing menus so they can be rebuilt.
    // This allows language changes of the menu text on the fly.
    menuBar->Freeze();

    while( menuBar->GetMenuCount() )
        delete menuBar->Remove( 0 );

    // Recreate all menus:

    // Create File Menu
    wxMenu* filesMenu = new wxMenu;

    if( Kiface().IsSingle() )   // not when under a project mgr
    {
        AddMenuItem( filesMenu, ID_NEW_BOARD,
                _( "&New" ),
                _( "Clear current board and initialize a new one" ),
                KiBitmap( new_pcb_xpm ) );

        text = AddHotkeyName( _( "&Open" ), m_hotkeysDescrList, HK_LOAD_BOARD );
        AddMenuItem( filesMenu, ID_LOAD_FILE, text,
                _( "Delete current board and load new board" ),
                KiBitmap( open_brd_file_xpm ) );
    }

    // Load Recent submenu
    static wxMenu* openRecentMenu;

    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentMenu )
        fhist.RemoveMenu( openRecentMenu );

    openRecentMenu = new wxMenu();

    fhist.UseMenu( openRecentMenu );
    fhist.AddFilesToMenu();

    if( Kiface().IsSingle() )      // not when under a project mgr
    {
        AddMenuItem( filesMenu, openRecentMenu,
                     -1, _( "Open &Recent" ),
                     _( "Open a recent opened board" ),
                     KiBitmap( open_project_xpm ) );
    }

    menutitem = AddMenuItem( filesMenu, ID_APPEND_FILE,
                 _( "&Append Board" ),
                 _( "Append another Pcbnew board to the current loaded board. Available only when Pcbnew runs in stand alone mode" ),
                 KiBitmap( import_xpm ) );
    if( ! Kiface().IsSingle() )      // disable when under a project mgr
        menutitem->Enable( false );


    filesMenu->AppendSeparator();

    text = AddHotkeyName( _( "&Save" ), m_hotkeysDescrList, HK_SAVE_BOARD );
    AddMenuItem( filesMenu, ID_SAVE_BOARD, text,
                 _( "Save current board" ),
                 KiBitmap( save_xpm ) );

    // Save as menu:
    // under a project mgr we do not want to modify the board filename
    // to keep consistency with the project mgr which expects files names same as prj name
    // for main files
    // when not under a project mgr, we are free to change filenames, cwd ...
    if( Kiface().IsSingle() )      // not when under a project mgr (pcbnew is run as stand alone)
    {
        text = AddHotkeyName( _( "Sa&ve As..." ), m_hotkeysDescrList, HK_SAVE_BOARD_AS );
        AddMenuItem( filesMenu, ID_SAVE_BOARD_AS, text,
                     _( "Save the current board as..." ),
                     KiBitmap( save_as_xpm ) );
    }
    // under a project mgr, we can save a copy of the board,
    // but do not change the current board file name
    else
    {
        text = AddHotkeyName( _( "Sa&ve Copy As..." ), m_hotkeysDescrList, HK_SAVE_BOARD_AS );
        AddMenuItem( filesMenu, ID_COPY_BOARD_AS, text,
                     _( "Save a copy of the current board as..." ),
                     KiBitmap( save_as_xpm ) );
    }

    filesMenu->AppendSeparator();

    AddMenuItem( filesMenu, ID_MENU_READ_BOARD_BACKUP_FILE,
                 _( "Revert to Las&t" ),
                 _( "Clear board and get previous backup version of board" ),
                 KiBitmap( revert_pcbnew_xpm ) );

    AddMenuItem( filesMenu, ID_MENU_RECOVER_BOARD_AUTOSAVE,
            _( "Resc&ue" ),
            _( "Clear board and get last rescue file automatically saved by Pcbnew" ),
            KiBitmap( rescue_pcbnew_xpm ) );
    filesMenu->AppendSeparator();

    //----- Fabrication Outputs submenu -----------------------------------------
    wxMenu* fabricationOutputsMenu = new wxMenu;
    AddMenuItem( fabricationOutputsMenu, ID_PCB_GEN_POS_MODULES_FILE,
                 _( "Footprint &Position (.pos) File" ),
                 _( "Generate footprint position file for pick and place" ),
                 KiBitmap( post_compo_xpm ) );

    AddMenuItem( fabricationOutputsMenu, ID_PCB_GEN_DRILL_FILE,
                 _( "&Drill (.drl) File" ),
                 _( "Generate excellon2 drill file" ),
                 KiBitmap( post_drill_xpm ) );

    AddMenuItem( fabricationOutputsMenu, ID_GEN_EXPORT_FILE_MODULE_REPORT,
                 _( "&Footprint (.rpt) Report" ),
                 _( "Create a report of all footprints on the current board" ),
                 KiBitmap( tools_xpm ) );

    AddMenuItem( fabricationOutputsMenu, ID_PCB_GEN_D356_FILE,
            _( "IPC-D-356 Netlist File" ),
            _( "Generate IPC-D-356 netlist file" ),
            KiBitmap( netlist_xpm ) );

    AddMenuItem( fabricationOutputsMenu, ID_PCB_GEN_CMP_FILE,
                 _( "&Component (.cmp) File" ),
                 _( "(Re)create components file (*.cmp) for CvPcb" ),
                 KiBitmap( create_cmp_file_xpm ) );

    AddMenuItem( fabricationOutputsMenu, ID_PCB_GEN_BOM_FILE_FROM_BOARD,
                 _( "&BOM File" ),
                 _( "Create a bill of materials from schematic" ),
                 KiBitmap( bom_xpm ) );

    AddMenuItem( filesMenu, fabricationOutputsMenu,
                 -1, _( "&Fabrication Outputs" ),
                 _( "Generate files for fabrication" ),
                 KiBitmap( fabrication_xpm ) );

    //----- Import submenu ------------------------------------------------------
    wxMenu* submenuImport = new wxMenu();

    AddMenuItem( submenuImport, ID_GEN_IMPORT_SPECCTRA_SESSION,
                 _( "&Specctra Session" ),
                 _( "Import a routed \"Specctra Session\" (*.ses) file" ),
                 KiBitmap( import_xpm ) );

    AddMenuItem( submenuImport, ID_GEN_IMPORT_DXF_FILE,
                 _( "&DXF File" ),
                 _( "Import a 2D Drawing DXF file to Pcbnew on the Drawings layer" ),
                 KiBitmap( import_xpm ) );

    AddMenuItem( filesMenu, submenuImport,
                 ID_GEN_IMPORT_FILE, _( "&Import" ),
                 _( "Import files" ), KiBitmap( import_xpm ) );


    //----- Export submenu ------------------------------------------------------
    wxMenu* submenuexport = new wxMenu();

    AddMenuItem( submenuexport, ID_GEN_EXPORT_SPECCTRA,
                 _( "&Specctra DSN" ),
                 _( "Export the current board to a \"Specctra DSN\" file" ),
                 KiBitmap( export_dsn_xpm ) );

    AddMenuItem( submenuexport, ID_GEN_EXPORT_FILE_GENCADFORMAT,
                 _( "&GenCAD" ), _( "Export GenCAD format" ),
                 KiBitmap( export_xpm ) );

    AddMenuItem( submenuexport, ID_GEN_EXPORT_FILE_VRML,
                 _( "&VRML" ),
                 _( "Export a VRML board representation" ),
                 KiBitmap( three_d_xpm ) );

    AddMenuItem( submenuexport, ID_GEN_EXPORT_FILE_IDF3,
                 _( "I&DFv3 Export" ), _( "IDFv3 board and component export" ),
                 KiBitmap( export_idf_xpm ) );

    AddMenuItem( filesMenu, submenuexport,
                 ID_GEN_EXPORT_FILE, _( "E&xport" ),
                 _( "Export board" ), KiBitmap( export_xpm ) );

    filesMenu->AppendSeparator();

    AddMenuItem( filesMenu, ID_SHEET_SET,
                 _( "Page s&ettings" ),
                 _( "Page settings for paper size and texts" ),
                 KiBitmap( sheetset_xpm ) );

    AddMenuItem( filesMenu, wxID_PRINT,
                 _( "&Print" ), _( "Print board" ),
                 KiBitmap( print_button_xpm ) );

    AddMenuItem( filesMenu, ID_GEN_PLOT_SVG,
                 _( "Export SV&G" ),
                 _( "Export a board file in Scalable Vector Graphics format" ),
                 KiBitmap( plot_svg_xpm ) );

    AddMenuItem( filesMenu, ID_GEN_PLOT,
                 _( "P&lot" ),
                 _( "Plot board in HPGL, PostScript or Gerber RS-274X format)" ),
                 KiBitmap( plot_xpm ) );

    filesMenu->AppendSeparator();

    //----- archive submenu -----------------------------------------------------
    wxMenu* submenuarchive = new wxMenu();

    AddMenuItem( submenuarchive, ID_MENU_ARCHIVE_NEW_MODULES,
                 _( "&Archive New Footprints" ),
                 _( "Archive new footprints only in a library (keep other footprints in this lib)" ),
                 KiBitmap( library_update_xpm ) );

    AddMenuItem( submenuarchive, ID_MENU_ARCHIVE_ALL_MODULES,
                 _( "&Create Footprint Archive" ),
                 _( "Archive all footprints in a library (old library will be deleted)" ),
                 KiBitmap( library_xpm ) );

    AddMenuItem( filesMenu, submenuarchive,
                 ID_MENU_ARCHIVE_MODULES,
                 _( "Arc&hive Footprints" ),
                 _( "Archive or add footprints in a library file" ),
                 KiBitmap( library_xpm ) );

    filesMenu->AppendSeparator();
    AddMenuItem( filesMenu, wxID_EXIT, _( "&Close" ), _( "Close Pcbnew" ), KiBitmap( exit_xpm ) );

    //----- Edit menu -----------------------------------------------------------
    wxMenu* editMenu = new wxMenu;

    text  = AddHotkeyName( _( "&Undo" ), g_Pcbnew_Editor_Hokeys_Descr, HK_UNDO );
    AddMenuItem( editMenu, wxID_UNDO, text, HELP_UNDO, KiBitmap( undo_xpm ) );

    text  = AddHotkeyName( _( "&Redo" ), g_Pcbnew_Editor_Hokeys_Descr, HK_REDO );
    AddMenuItem( editMenu, wxID_REDO, text, HELP_REDO, KiBitmap( redo_xpm ) );

    AddMenuItem( editMenu, ID_PCB_DELETE_ITEM_BUTT,
                 _( "&Delete" ), _( "Delete items" ),
                 KiBitmap( delete_xpm ) );

    editMenu->AppendSeparator();

    text = AddHotkeyName( _( "&Find" ), g_Pcbnew_Editor_Hokeys_Descr, HK_FIND_ITEM );
    AddMenuItem( editMenu, ID_FIND_ITEMS, text, HELP_FIND , KiBitmap( find_xpm ) );

    editMenu->AppendSeparator();

    AddMenuItem( editMenu, ID_PCB_GLOBAL_DELETE,
                 _( "&Global Deletions" ),
                 _( "Delete tracks, footprints, texts... on board" ),
                 KiBitmap( general_deletions_xpm ) );

    AddMenuItem( editMenu, ID_MENU_PCB_CLEAN,
                 _( "&Cleanup Tracks and Vias" ),
                 _( "Clean stubs, vias, delete break points, or connect dangling tracks to pads and vias" ),
                 KiBitmap( delete_xpm ) );

    AddMenuItem( editMenu, ID_MENU_PCB_SWAP_LAYERS,
                 _( "&Swap Layers" ),
                 _( "Swap tracks on copper layers or drawings on other layers" ),
                 KiBitmap( swap_layer_xpm ) );

    AddMenuItem( editMenu, ID_MENU_PCB_RESET_TEXTMODULE_FIELDS_SIZES,
                 _( "&Reset Footprint Field Sizes" ),
                 _( "Reset text size and width of all footprint fields to current defaults" ),
                 KiBitmap( reset_text_xpm ) );

    //----- View menu -----------------------------------------------------------
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
    text = AddHotkeyName( _( "Zoom &In" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ZOOM_IN, IS_ACCELERATOR );
    AddMenuItem( viewMenu, ID_ZOOM_IN, text, HELP_ZOOM_IN, KiBitmap( zoom_in_xpm ) );

    text = AddHotkeyName( _( "Zoom &Out" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ZOOM_OUT, IS_ACCELERATOR );
    AddMenuItem( viewMenu, ID_ZOOM_OUT, text, HELP_ZOOM_OUT, KiBitmap( zoom_out_xpm ) );

    text = AddHotkeyName( _( "&Fit on Screen" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ZOOM_AUTO  );
    AddMenuItem( viewMenu, ID_ZOOM_PAGE, text, HELP_ZOOM_FIT,
                 KiBitmap( zoom_fit_in_page_xpm ) );

    text = AddHotkeyName( _( "&Redraw" ), g_Pcbnew_Editor_Hokeys_Descr, HK_ZOOM_REDRAW );
    AddMenuItem( viewMenu, ID_ZOOM_REDRAW, text,
                 HELP_ZOOM_REDRAW, KiBitmap( zoom_redraw_xpm ) );

    viewMenu->AppendSeparator();

    text = AddHotkeyName( _( "&3D Viewer" ), g_Pcbnew_Editor_Hokeys_Descr, HK_3D_VIEWER );

    AddMenuItem( viewMenu, ID_MENU_PCB_SHOW_3D_FRAME, text, _( "Show board in 3D viewer" ),
                 KiBitmap( three_d_xpm ) );

    AddMenuItem( viewMenu, ID_MENU_LIST_NETS,
                 _( "&List Nets" ), _( "View a list of nets with names and id's" ),
                 KiBitmap( list_nets_xpm ) );

    viewMenu->AppendSeparator();

    text = AddHotkeyName( _( "&Switch canvas to default" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_CANVAS_DEFAULT );

    AddMenuItem( viewMenu, ID_MENU_CANVAS_DEFAULT,
                 text, _( "Switch the canvas implementation to default" ),
                 KiBitmap( tools_xpm ) );

    text = AddHotkeyName( _( "Switch canvas to Open&GL" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_CANVAS_OPENGL );

    AddMenuItem( viewMenu, ID_MENU_CANVAS_OPENGL,
                 text, _( "Switch the canvas implementation to OpenGL" ),
                 KiBitmap( tools_xpm ) );

    text = AddHotkeyName( _( "Switch canvas to &Cairo" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_CANVAS_CAIRO );

    AddMenuItem( viewMenu, ID_MENU_CANVAS_CAIRO,
                 text, _( "Switch the canvas implementation to Cairo" ),
                 KiBitmap( tools_xpm ) );

    //----- Place Menu ----------------------------------------------------------
    wxMenu* placeMenu = new wxMenu;

    text = AddHotkeyName( _( "&Footprint" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ADD_MODULE );
    AddMenuItem( placeMenu, ID_PCB_MODULE_BUTT, text,
                 _( "Add footprints" ), KiBitmap( module_xpm ) );

    text = AddHotkeyName( _( "&Track" ), g_Pcbnew_Editor_Hokeys_Descr,
                          HK_ADD_NEW_TRACK );
    AddMenuItem( placeMenu, ID_TRACK_BUTT, text,
                 _( "Add tracks and vias" ), KiBitmap( add_tracks_xpm ) );

    AddMenuItem( placeMenu, ID_PCB_ZONES_BUTT,
                 _( "&Zone" ), _( "Add filled zones" ), KiBitmap( add_zone_xpm ) );

    AddMenuItem( placeMenu, ID_PCB_KEEPOUT_AREA_BUTT,
                 _( "&Keepout Area" ), _( "Add keepout areas" ), KiBitmap( add_keepout_area_xpm ) );

    AddMenuItem( placeMenu, ID_PCB_ADD_TEXT_BUTT,
                 _( "Te&xt" ), _( "Add text on copper layers or graphic text" ),
                 KiBitmap( add_text_xpm ) );

    AddMenuItem( placeMenu, ID_PCB_ARC_BUTT,
                 _( "&Arc" ), _( "Add graphic arc" ),KiBitmap( add_arc_xpm ) );

    AddMenuItem( placeMenu, ID_PCB_CIRCLE_BUTT,
                 _( "&Circle" ), _( "Add graphic circle" ),
                 KiBitmap( add_circle_xpm ) );

    AddMenuItem( placeMenu, ID_PCB_ADD_LINE_BUTT,
                 _( "&Line or Polygon" ),
                 _( "Add graphic line or polygon" ),
                 KiBitmap( add_dashed_line_xpm ) );

    placeMenu->AppendSeparator();

    AddMenuItem( placeMenu, ID_PCB_DIMENSION_BUTT,
                 _( "&Dimension" ), _( "Add dimension" ),
                 KiBitmap( add_dimension_xpm ) );

    AddMenuItem( placeMenu, ID_PCB_MIRE_BUTT,
                 _( "La&yer alignment target" ), _( "Add layer alignment target" ),
                 KiBitmap( add_mires_xpm ) );

    placeMenu->AppendSeparator();

    AddMenuItem( placeMenu, ID_PCB_PLACE_OFFSET_COORD_BUTT,
                 _( "Drill and &Place Offset" ),
                 _( "Place the origin point for drill and place files" ),
                 KiBitmap( pcb_offset_xpm ) );

    AddMenuItem( placeMenu, ID_PCB_PLACE_GRID_COORD_BUTT,
                 _( "&Grid Origin" ),
                 _( "Set the origin point for the grid" ),
                 KiBitmap( grid_select_axis_xpm ) );

    wxMenu* routeMenu = new wxMenu;

    AddMenuItem( routeMenu, ID_TRACK_BUTT,
                 _( "&Single Track" ),
                 _( "Interactively route a single track" ),
                 KiBitmap( add_tracks_xpm ) );

    AddMenuItem( routeMenu, ID_DIFF_PAIR_BUTT,
                 _( "&Differential Pair" ),
                 _( "Interactively route a differential pair" ),
                 KiBitmap( ps_diff_pair_xpm ) );

    routeMenu->AppendSeparator();

    AddMenuItem( routeMenu, ID_TUNE_SINGLE_TRACK_LEN_BUTT,
                 _( "&Tune Track Length" ),
                 _( "Tune length of a single track" ),
                 KiBitmap( ps_tune_length_xpm ) );

    AddMenuItem( routeMenu, ID_TUNE_DIFF_PAIR_LEN_BUTT,
                 _( "Tune Differential Pair &Length" ),
                 _( "Tune length of a differential pair" ),
                 KiBitmap( ps_diff_pair_tune_length_xpm ) );

    AddMenuItem( routeMenu, ID_TUNE_DIFF_PAIR_SKEW_BUTT,
                 _( "Tune Differential Pair &Skew/Phase" ),
                 _( "Tune skew/phase of a differential pair" ),
                 KiBitmap( ps_diff_pair_tune_phase_xpm ) );

/* Fixme: add icons & missing menu entries!
    routeMenu->AppendSeparator();

    AddMenuItem( routeMenu, ID_MENU_MITER_TRACES,
                 _( "Miter traces..." ),
                 _( "Miter trace corners with arcs" ),
                 KiBitmap( grid_select_axis_xpm ) );

    AddMenuItem( routeMenu, ID_MENU_ADD_TEARDROPS,
                 _( "Teardrops..." ),
                 _( "Add teardrops to pads/vias" ),
                 KiBitmap( grid_select_axis_xpm ) );
*/

    //----- Preferences and configuration menu------------------------------------
    wxMenu* configmenu = new wxMenu;

    AddMenuItem( configmenu, ID_PCB_LIB_WIZARD,
                _( "&Footprint Libraries Wizard" ), _( "Add footprint libraries with wizard" ),
                KiBitmap( wizard_add_fplib_small_xpm ) );

    AddMenuItem( configmenu, ID_PCB_LIB_TABLE_EDIT,
                _( "Footprint Li&braries Manager" ), _( "Configure footprint libraries" ),
                KiBitmap( library_table_xpm ) );

    // Path configuration edit dialog.
    AddMenuItem( configmenu,
                 ID_PREFERENCES_CONFIGURE_PATHS,
                 _( "Configure Pa&ths" ),
                 _( "Edit path configuration environment variables" ),
                 KiBitmap( editor_xpm ) );

#ifdef BUILD_GITHUB_PLUGIN
    AddMenuItem( configmenu, ID_PCB_3DSHAPELIB_WIZARD,
                 _( "&3D Shapes Libraries Downloader" ),
                 _( "Download from Github the 3D shape libraries with wizard" ),
                 KiBitmap( wizard_add_fplib_small_xpm ) );
#endif

    // Colors and Visibility are also handled by the layers manager toolbar
    AddMenuItem( configmenu, ID_MENU_PCB_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
                 m_show_layer_manager_tools ?
                 _( "Hide La&yers Manager" ) : _("Show La&yers Manager" ),
                 HELP_SHOW_HIDE_LAYERMANAGER,
                 KiBitmap( layers_manager_xpm ) );

    AddMenuItem( configmenu, ID_MENU_PCB_SHOW_HIDE_MUWAVE_TOOLBAR,
                 m_show_microwave_tools ?
                 _( "Hide Microwa&ve Toolbar" ): _( "Show Microwave Toolbar" ),
                 HELP_SHOW_HIDE_MICROWAVE_TOOLS,
                 KiBitmap( mw_toolbar_xpm ) );

    // General
#ifdef __WXMAC__
    configmenu->Append( wxID_PREFERENCES );
#else
    AddMenuItem( configmenu, wxID_PREFERENCES,
                 _( "&General" ), _( "Select general options for Pcbnew" ),
                 KiBitmap( preference_xpm ) );
#endif

    AddMenuItem( configmenu, ID_PCB_DISPLAY_OPTIONS_SETUP,
                 _( "&Display" ),
                 _( "Select how items (pads, tracks texts ... ) are displayed" ),
                 KiBitmap( display_options_xpm ) );

    AddMenuItem( configmenu, ID_MENU_INTERACTIVE_ROUTER_SETTINGS,
                 _( "&Interactive Routing" ),
                 _( "Configure Interactive Routing." ),
                 KiBitmap( add_tracks_xpm ) ); // fixme: icon

    //--- dimensions submenu ------------------------------------------------------
    wxMenu* dimensionsMenu = new wxMenu;

    AddMenuItem( dimensionsMenu, ID_PCB_USER_GRID_SETUP,
                 _( "G&rid" ),_( "Adjust user grid dimensions" ),
                 KiBitmap( grid_xpm ) );

    AddMenuItem( dimensionsMenu, ID_PCB_DRAWINGS_WIDTHS_SETUP,
                 _( "Te&xts and Drawings" ),
                 _( "Adjust dimensions for texts and drawings" ),
                 KiBitmap( options_text_xpm ) );

    AddMenuItem( dimensionsMenu, ID_PCB_PAD_SETUP,
                 _( "&Pads" ),  _( "Adjust default pad characteristics" ),
                 KiBitmap( pad_dimensions_xpm ) );

    AddMenuItem( dimensionsMenu, ID_PCB_MASK_CLEARANCE,
                 _( "Pads &Mask Clearance" ),
                 _( "Adjust the global clearance between pads and the solder resist mask" ),
                 KiBitmap( pads_mask_layers_xpm ) );

    AddMenuItem( dimensionsMenu, ID_MENU_DIFF_PAIR_DIMENSIONS,
                 _( "&Differential Pairs" ),
                 _( "Define the global gap/width for differential pairs." ),
                 KiBitmap( ps_diff_pair_xpm ) );

    dimensionsMenu->AppendSeparator();
    AddMenuItem( dimensionsMenu, ID_CONFIG_SAVE,
                 _( "&Save" ), _( "Save dimension preferences" ),
                 KiBitmap( save_xpm ) );

    // Language submenu
    Pgm().AddMenuLanguageList( configmenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( configmenu );

    //--- Macros submenu --------------------------------------------------------
    wxMenu* macrosMenu = new wxMenu;

    AddMenuItem( macrosMenu, ID_PREFRENCES_MACROS_SAVE,
                 _( "&Save macros" ),
                 _( "Save macros to file" ),
                 KiBitmap( save_setup_xpm ) );

    AddMenuItem( macrosMenu, ID_PREFRENCES_MACROS_READ,
                 _( "&Read macros" ),
                 _( "Read macros from file" ),
                 KiBitmap( read_setup_xpm ) );

    AddMenuItem( configmenu, macrosMenu,
                 -1, _( "Ma&cros" ),
                 _( "Macros save/read operations" ),
                 KiBitmap( macros_record_xpm ) );

    configmenu->AppendSeparator();

    AddMenuItem( configmenu, ID_CONFIG_SAVE,
                 _( "&Save Preferences" ),
                 _( "Save application preferences" ),
                 KiBitmap( save_setup_xpm ) );

    AddMenuItem( configmenu, ID_CONFIG_READ,
                 _( "Load Prefe&rences" ),
                 _( "Load application preferences" ),
                 KiBitmap( read_setup_xpm ) );

    //----- Tools menu ----------------------------------------------------------
    wxMenu* toolsMenu = new wxMenu;

    AddMenuItem( toolsMenu, ID_GET_NETLIST,
                 _( "&Netlist" ),
                 _( "Read the netlist and update board connectivity" ),
                 KiBitmap( netlist_xpm ) );

    AddMenuItem( toolsMenu, ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR,
                 _( "&Layer Pair" ), _( "Change the active layer pair" ),
                 KiBitmap( select_layer_pair_xpm ) );

    AddMenuItem( toolsMenu, ID_DRC_CONTROL,
                 _( "&DRC" ),
                 _( "Perform design rules check" ), KiBitmap( erc_xpm ) );

    AddMenuItem( toolsMenu, ID_TOOLBARH_PCB_FREEROUTE_ACCESS,
                 _( "&FreeRoute" ),
                 _( "Fast access to the Web Based FreeROUTE advanced router" ),
                 KiBitmap( web_support_xpm ) );

#if defined(KICAD_SCRIPTING_WXPYTHON)
    AddMenuItem( toolsMenu, ID_TOOLBARH_PCB_SCRIPTING_CONSOLE,
                 _( "&Scripting Console" ),
                 _( "Show/Hide the Python Scripting console" ),
                 KiBitmap( py_script_xpm ) );
#endif

    wxMenu* designRulesMenu = new wxMenu;

    AddMenuItem( designRulesMenu, ID_MENU_PCB_SHOW_DESIGN_RULES_DIALOG,
                 _( "&Design Rules" ),
                 _( "Open the design rules editor" ), KiBitmap( hammer_xpm ) );

    AddMenuItem( designRulesMenu, ID_PCB_LAYERS_SETUP,
                 _( "&Layers Setup" ),  _( "Enable and set layer properties" ),
                 KiBitmap( copper_layers_setup_xpm ) );

    wxMenu* helpMenu = new wxMenu;

    AddHelpVersionInfoMenuEntry( helpMenu );

    AddMenuItem( helpMenu, wxID_HELP,
                 _( "&Contents" ),
                 _( "Open the Pcbnew handbook" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu, wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open the \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    helpMenu->AppendSeparator();
    AddMenuItem( helpMenu, wxID_ABOUT,
                 _( "&About Pcbnew" ),
                 _( "About Pcbnew printed circuit board designer" ),
                 KiBitmap( info_xpm ) );

    // Append all menus to the menuBar
    menuBar->Append( filesMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( routeMenu, _( "Ro&ute" ) );
    menuBar->Append( configmenu, _( "P&references" ) );
    menuBar->Append( dimensionsMenu, _( "D&imensions" ) );
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
