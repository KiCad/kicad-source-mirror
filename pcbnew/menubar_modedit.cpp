/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcbnew/menubar_modedit.cpp
 * @brief (Re)Create the main menubar for the module editor
 */


#include <menus_helpers.h>
#include <pgm_base.h>
#include <wxPcbStruct.h>


#include "help_common_strings.h"
#include "hotkeys.h"
#include "module_editor_frame.h"
#include "pcbnew.h"
#include "pcbnew_id.h"


void FOOTPRINT_EDIT_FRAME::ReCreateMenuBar()
{
    // Create and try to get the current menubar
    wxMenuBar* menuBar = GetMenuBar();

    if( !menuBar )
        menuBar = new wxMenuBar();

    // Delete all existing menus so they can be rebuilt.
    // This allows language changes of the menu text on the fly.
    menuBar->Freeze();

    while( menuBar->GetMenuCount() )
        delete menuBar->Remove( 0 );

    // Recreate all menus:
    wxString text;

    // Menu File:
    wxMenu* fileMenu = new wxMenu;

    // Active library selection
    AddMenuItem( fileMenu, ID_MODEDIT_SELECT_CURRENT_LIB, _("Set Acti&ve Library"),
                           _( "Select active library" ),
                           KiBitmap( open_library_xpm ) );
    fileMenu->AppendSeparator();

    // New module
    AddMenuItem( fileMenu, ID_MODEDIT_NEW_MODULE,
                 _( "&New Footprint" ), _( "Create new footprint" ),
                 KiBitmap( new_footprint_xpm ) );

    // Open submenu
    wxMenu* openSubmenu = new wxMenu;

    // from File
    AddMenuItem( openSubmenu, ID_MODEDIT_IMPORT_PART,
                 _( "&Import Footprint From File" ),
                 _( "Import footprint from an existing file" ),
                 KiBitmap( import_module_xpm ) );

    // from Library
    AddMenuItem( openSubmenu, ID_MODEDIT_LOAD_MODULE,
                 _( "Load Footprint From Current Li&brary" ),
                 _( "Open footprint from library" ),
                 KiBitmap( module_xpm ) );

    // from current Board
    AddMenuItem( openSubmenu, ID_MODEDIT_LOAD_MODULE_FROM_BOARD,
                 _( "Load Footprint From &Current Board" ),
                 _( "Load footprint from current board" ),
                 KiBitmap( load_module_board_xpm ) );

    /* Append openSubmenu to fileMenu */
    AddMenuItem( fileMenu, openSubmenu, -1,
                 _( "&Load Footprint" ),
                 _( "Load footprint" ),
                 KiBitmap( open_document_xpm ) );
    fileMenu->AppendSeparator();

    // Save the currently loaded legacy library as an s-expression library.
    AddMenuItem( fileMenu, ID_MODEDIT_SAVE_LIBRARY_AS,
                 _( "Save &Current Library As..." ),
                 _( "Save entire current library under a new name" ),
                 KiBitmap( copy_library_xpm ) );

    // Save module
    text = AddHotkeyName( _( "&Save Footprint in Active Library" ),
                          m_hotkeysDescrList, HK_SAVE_MODULE );

    AddMenuItem( fileMenu, ID_MODEDIT_SAVE_LIBMODULE, text,
                 _( "Save footprint in active library" ),
                 KiBitmap( save_library_xpm ) );

    // Save module in new lib
    AddMenuItem( fileMenu, ID_MODEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART,
                 _( "S&ave Footprint in New Library" ),
                 _( "Create new library and save current footprint in it" ),
                 KiBitmap( new_library_xpm ) );

    // Export module
    AddMenuItem( fileMenu, ID_MODEDIT_EXPORT_PART,
                 _( "&Export Footprint" ),
                 _( "Save currently loaded footprint into file" ),
                 KiBitmap( export_module_xpm ) );

    // Import DXF File
    AddMenuItem( fileMenu, ID_GEN_IMPORT_DXF_FILE,
                 _( "&Import DXF File" ),
                 _( "Import 2D Drawing DXF file to Pcbnew on Drawings layer" ),
                 KiBitmap( import_xpm ) );

    fileMenu->AppendSeparator();

    // Print
    AddMenuItem( fileMenu, wxID_PRINT,
                 _( "&Print" ),
                 _( "Print current footprint" ),
                 KiBitmap( plot_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Close editor
    AddMenuItem( fileMenu, wxID_EXIT,
                 _( "Cl&ose" ),
                 _( "Close footprint editor" ),
                 KiBitmap( exit_xpm ) );

    //----- Edit menu ------------------
    wxMenu* editMenu = new wxMenu;

    // Undo
    text = AddHotkeyName( _( "&Undo" ), m_hotkeysDescrList, HK_UNDO );
    AddMenuItem( editMenu, wxID_UNDO,
                 text, _( "Undo last action" ),
                 KiBitmap( undo_xpm ) );

    // Redo
    text = AddHotkeyName( _( "&Redo" ), m_hotkeysDescrList, HK_REDO );
    AddMenuItem( editMenu, wxID_REDO,
                 text, _( "Redo last action" ),
                 KiBitmap( redo_xpm ) );

    // Delete items
    AddMenuItem( editMenu, ID_MODEDIT_DELETE_TOOL,
                 _( "&Delete" ), _( "Delete objects with eraser" ),
                 KiBitmap( delete_xpm ) );

    // Separator
    editMenu->AppendSeparator();

    // Properties
    AddMenuItem( editMenu, ID_MODEDIT_EDIT_MODULE_PROPERTIES,
                 _( "Footprint &Properties" ),
                 _( "Edit footprint properties" ),
                 KiBitmap( module_options_xpm ) );

    // Dimensions submenu
    wxMenu* dimensions_Submenu = new wxMenu;

    // User grid size
    AddMenuItem( dimensions_Submenu, ID_PCB_USER_GRID_SETUP,
                 _( "Set Custom &User Grid" ), _( "Set custom user grid size" ),
                 KiBitmap( grid_xpm ) );

    // Sizes and Widths
    AddMenuItem( dimensions_Submenu, ID_PCB_DRAWINGS_WIDTHS_SETUP,
                 _( "Text &Size and Width" ),
                 _( "Adjust width for text and drawing" ),
                 KiBitmap( text_xpm ) );

    // Pad settings
    AddMenuItem( dimensions_Submenu, ID_MODEDIT_PAD_SETTINGS,
                 _( "&Pad Properties" ), _( "Edit settings for new pads" ),
                 KiBitmap( pad_dimensions_xpm ) );

    //--------- View menu ----------------
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
    text = AddHotkeyName( _( "Zoom &In" ), m_hotkeysDescrList,
                          HK_ZOOM_IN, IS_ACCELERATOR );
    AddMenuItem( viewMenu, ID_ZOOM_IN, text, HELP_ZOOM_IN, KiBitmap( zoom_in_xpm ) );

    text = AddHotkeyName( _( "Zoom &Out" ), m_hotkeysDescrList,
                          HK_ZOOM_OUT, IS_ACCELERATOR );
    AddMenuItem( viewMenu, ID_ZOOM_OUT, text, HELP_ZOOM_OUT, KiBitmap( zoom_out_xpm ) );

    text = AddHotkeyName( _( "&Fit on Screen" ), m_hotkeysDescrList,
                          HK_ZOOM_AUTO  );
    AddMenuItem( viewMenu, ID_ZOOM_PAGE, text, HELP_ZOOM_FIT,
                 KiBitmap( zoom_fit_in_page_xpm ) );

    text = AddHotkeyName( _( "&Redraw" ), m_hotkeysDescrList, HK_ZOOM_REDRAW );
    AddMenuItem( viewMenu, ID_ZOOM_REDRAW, text,
                 HELP_ZOOM_REDRAW, KiBitmap( zoom_redraw_xpm ) );

    viewMenu->AppendSeparator();

    // 3D view
    text = AddHotkeyName( _( "&3D Viewer" ), m_hotkeysDescrList, HK_3D_VIEWER );
    AddMenuItem( viewMenu, ID_MENU_PCB_SHOW_3D_FRAME, text, _( "Show footprint in 3D viewer" ),
                 KiBitmap( three_d_xpm ) );

    // Add canvas selection
    viewMenu->AppendSeparator();

    text = AddHotkeyName( _( "Legacy Canva&s" ), m_hotkeysDescrList,
                          HK_CANVAS_LEGACY );

    viewMenu->Append(
        new wxMenuItem( viewMenu, ID_MENU_CANVAS_LEGACY,
                        text, _( "Switch the canvas implementation to Legacy" ),
                        wxITEM_RADIO ) );

    text = AddHotkeyName( _( "Open&GL Canvas" ), m_hotkeysDescrList,
                          HK_CANVAS_OPENGL );

    viewMenu->Append(
        new wxMenuItem( viewMenu, ID_MENU_CANVAS_OPENGL,
                        text, _( "Switch the canvas implementation to OpenGL" ),
                        wxITEM_RADIO ) );

    text = AddHotkeyName( _( "&Cairo Canvas" ), m_hotkeysDescrList,
                          HK_CANVAS_CAIRO );

    viewMenu->Append(
        new wxMenuItem( viewMenu, ID_MENU_CANVAS_CAIRO,
                        text, _( "Switch the canvas implementation to Cairo" ),
                        wxITEM_RADIO ) );

    //-------- Place menu --------------------
    wxMenu* placeMenu = new wxMenu;

    // Pad
    AddMenuItem( placeMenu, ID_MODEDIT_PAD_TOOL,
                 _( "&Pad" ), _( "Add pad" ),
                 KiBitmap( pad_xpm ) );

    placeMenu->AppendSeparator();

    // Circle
    AddMenuItem( placeMenu, ID_MODEDIT_CIRCLE_TOOL,
                 _( "&Circle" ), _( "Add graphic circle" ),
                 KiBitmap( add_circle_xpm ) );

    // Line or Polygon
    AddMenuItem( placeMenu, ID_MODEDIT_LINE_TOOL,
                 _( "&Line or Polygon" ),
                 _( "Add graphic line or polygon" ),
                 KiBitmap( add_polygon_xpm ) );

    // Arc
    AddMenuItem( placeMenu, ID_MODEDIT_ARC_TOOL,
                 _( "&Arc" ), _( "Add graphic arc" ),
                 KiBitmap( add_arc_xpm ) );

    // Text
    AddMenuItem( placeMenu, ID_MODEDIT_TEXT_TOOL,
                 _( "&Text" ), _( "Add graphic text" ),
                 KiBitmap( text_xpm ) );

    placeMenu->AppendSeparator();

    // Anchor
    AddMenuItem( placeMenu, ID_MODEDIT_ANCHOR_TOOL,
                 _( "A&nchor" ),
                 _( "Place footprint reference anchor" ),
                 KiBitmap( anchor_xpm ) );


    //----- Preferences menu -----------------
    wxMenu* prefs_menu = new wxMenu;

    AddMenuItem( prefs_menu, ID_PCB_LIB_WIZARD,
                _( "&Footprint Libraries Wizard" ), _( "Add footprint libraries with wizard" ),
                KiBitmap( wizard_add_fplib_small_xpm ) );

    AddMenuItem( prefs_menu, ID_PCB_LIB_TABLE_EDIT,
                _( "Footprint Li&braries Manager" ), _( "Configure footprint libraries" ),
                KiBitmap( library_table_xpm ) );

    // Path configuration edit dialog.
    AddMenuItem( prefs_menu,
                 ID_PREFERENCES_CONFIGURE_PATHS,
                 _( "Configure Pa&ths" ),
                 _( "Edit path configuration environment variables" ),
                 KiBitmap( path_xpm ) );

    // Settings
    AddMenuItem( prefs_menu, wxID_PREFERENCES,
                 _( "General &Settings" ), _( "Change footprint editor settings." ),
                 KiBitmap( preference_xpm ) );

    AddMenuItem( prefs_menu, ID_PCB_DISPLAY_OPTIONS_SETUP,
                 _( "&Display and Hide" ),
                 _( "Change footprint editor display settings" ),
                 KiBitmap( display_options_xpm ) );

    // Language submenu
    Pgm().AddMenuLanguageList( prefs_menu );

    // Hotkey submenu
    AddHotkeyConfigMenu( prefs_menu );

    //----- Help menu --------------------
    wxMenu* helpMenu = new wxMenu;

    // Contents
    AddMenuItem( helpMenu, wxID_HELP,
                 _( "Pcbnew &Manual" ),
                 _( "Open the Pcbnew Manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu, wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open the \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    AddMenuItem( helpMenu, ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST,
                 _( "&List Hotkeys" ),
                 _( "Displays current hotkeys table and corresponding commands" ),
                 KiBitmap( hotkeys_xpm ) );

    helpMenu->AppendSeparator();

    AddMenuItem( helpMenu, ID_HELP_GET_INVOLVED,
                 _( "Get &Involved" ),
                 _( "Contribute to KiCad (open web browser)" ),
                 KiBitmap( info_xpm ) );

    // About Pcbnew
    helpMenu->AppendSeparator();
    AddMenuItem( helpMenu, wxID_ABOUT,
                 _( "&About KiCad" ),
                 _( "About KiCad" ),
                 KiBitmap( about_xpm ) );

    // Append menus to the menubar
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( prefs_menu, _( "P&references" ) );
    menuBar->Append( dimensions_Submenu, _( "Di&mensions" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}
