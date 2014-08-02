
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <fctsys.h>

#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <module_editor_frame.h>
#include <menus_helpers.h>

#include <pcbnew_id.h>
#include <hotkeys.h>


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
    AddMenuItem( fileMenu, ID_MODEDIT_SELECT_CURRENT_LIB, _("Set Active Library"),
                           _( "Select active library" ),
                           KiBitmap( open_library_xpm ) );
    fileMenu->AppendSeparator();

    // New module
    AddMenuItem( fileMenu, ID_MODEDIT_NEW_MODULE,
                 _( "&New Module" ), _( "Create new module" ),
                 KiBitmap( new_footprint_xpm ) );

    // Open submenu
    wxMenu* openSubmenu = new wxMenu;

    // from File
    AddMenuItem( openSubmenu, ID_MODEDIT_IMPORT_PART,
                 _( "&Import Module From File" ),
                 _( "Import footprint from an existing file" ),
                 KiBitmap( import_module_xpm ) );

    // from Library
    AddMenuItem( openSubmenu, ID_MODEDIT_LOAD_MODULE,
                 _( "Load Module From Current Li&brary" ),
                 _( "Open a footprint module from library" ),
                 KiBitmap( module_xpm ) );

    // from current Board
    AddMenuItem( openSubmenu, ID_MODEDIT_LOAD_MODULE_FROM_BOARD,
                 _( "Load Module From &Current Board" ),
                 _( "Load a footprint module from the current board" ),
                 KiBitmap( load_module_board_xpm ) );

    /* Append openSubmenu to fileMenu */
    AddMenuItem( fileMenu, openSubmenu, -1,
                 _( "&Load Module" ),
                 _( "Load footprint module" ),
                 KiBitmap( open_document_xpm ) );
    fileMenu->AppendSeparator();

    // Save the currently loaded legacy library as an s-expression library.
    AddMenuItem( fileMenu, ID_MODEDIT_SAVE_LIBRARY_AS,
                 _( "Save Current Library As..." ),
                 _( "Save entire current library under a new name." ),
                 wxNullBitmap );

    // Save module
    text = AddHotkeyName( _( "&Save Module in Active Library" ),
                          g_Module_Editor_Hokeys_Descr, HK_SAVE_MODULE );

    AddMenuItem( fileMenu, ID_MODEDIT_SAVE_LIBMODULE, text,
                 _( "Save module in active library" ),
                 KiBitmap( save_library_xpm ) );

    // Save module in new lib
    AddMenuItem( fileMenu, ID_MODEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART,
                 _( "S&ave Module in New Library" ),
                 _( "Create a new library and save current module into it" ),
                 KiBitmap( new_library_xpm ) );

    // Export module
    AddMenuItem( fileMenu, ID_MODEDIT_EXPORT_PART,
                 _( "&Export Module" ),
                 _( "Save current loaded module into file" ),
                 KiBitmap( export_module_xpm ) );
    fileMenu->AppendSeparator();

    // Print
    AddMenuItem( fileMenu, wxID_PRINT,
                 _( "&Print" ),
                 _( "Print current module" ),
                 KiBitmap( plot_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Close editor
    AddMenuItem( fileMenu, wxID_EXIT,
                 _( "Cl&ose" ),
                 _( "Close footprint editor" ),
                 KiBitmap( exit_xpm ) );

    // Menu Edit:
    wxMenu* editMenu = new wxMenu;

    // Undo
    text = AddHotkeyName( _( "&Undo" ), g_Module_Editor_Hokeys_Descr, HK_UNDO );
    AddMenuItem( editMenu, wxID_UNDO,
                 text, _( "Undo last action" ),
                 KiBitmap( undo_xpm ) );

    // Redo
    text = AddHotkeyName( _( "&Redo" ), g_Module_Editor_Hokeys_Descr, HK_REDO );
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
                 _( "Edit &Properties" ),
                 _( "Edit module properties" ),
                 KiBitmap( module_options_xpm ) );

    // Dimensions submenu
    wxMenu* dimensions_Submenu = new wxMenu;

    // Sizes and Widths
    AddMenuItem( dimensions_Submenu, ID_PCB_DRAWINGS_WIDTHS_SETUP,
                 _( "&Size and Width" ),
                 _( "Adjust width for texts and drawings" ),
                 KiBitmap( options_text_xpm ) );

    // Pad settings
    AddMenuItem( dimensions_Submenu, ID_MODEDIT_PAD_SETTINGS,
                 _( "&Pad Setting" ), _( "Edit settings for new pads" ),
                 KiBitmap( pad_dimensions_xpm ) );

    // User grid size
    AddMenuItem( dimensions_Submenu, ID_PCB_USER_GRID_SETUP,
                 _( "&User Grid Size" ), _( "Adjust user grid" ),
                 KiBitmap( grid_xpm ) );

    // View menu
    wxMenu* viewMenu = new wxMenu;

    // Zoom In
    AddMenuItem( viewMenu, ID_ZOOM_IN,
                 _( "Zoom &In" ), _( "Zoom in" ),
                 KiBitmap( zoom_in_xpm ) );

    // Zoom Out
    AddMenuItem( viewMenu, ID_ZOOM_OUT,
                 _( "Zoom &Out" ), _( "Zoom out" ),
                 KiBitmap( zoom_out_xpm ) );

    // Fit on Screen
    AddMenuItem( viewMenu, ID_ZOOM_PAGE,
                 _( "&Fit on Screen" ),
                 _( "Zoom to fit the module in the window" ),
                 KiBitmap( zoom_fit_in_page_xpm ) );

    viewMenu->AppendSeparator();

    // Redraw
    AddMenuItem( viewMenu, ID_ZOOM_REDRAW,
                 _( "&Redraw" ), _( "Redraw window's viewport" ),
                 KiBitmap( zoom_redraw_xpm ) );

    // 3D view
    AddMenuItem( viewMenu, ID_MENU_PCB_SHOW_3D_FRAME,
                 _( "3&D View" ),
                 _( "Show board in 3D viewer" ),
                 KiBitmap( three_d_xpm ) );

    // Menu Place:
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
                 KiBitmap( add_text_xpm ) );

    placeMenu->AppendSeparator();

    // Anchor
    AddMenuItem( placeMenu, ID_MODEDIT_ANCHOR_TOOL,
                 _( "A&nchor" ),
                 _( "Place footprint module reference anchor" ),
                 KiBitmap( anchor_xpm ) );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contents
    AddMenuItem( helpMenu, wxID_HELP,
                 _( "P&cbnew Manual" ),
                 _( "Open the Pcbnew manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu, wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open the \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    // About Pcbnew
    helpMenu->AppendSeparator();
    AddMenuItem( helpMenu, wxID_ABOUT,
                 _( "&About Pcbnew" ),
                 _( "About Pcbnew PCB designer" ),
                 KiBitmap( info_xpm ) );

    // Append menus to the menubar
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( dimensions_Submenu, _( "Di&mensions" ) );

    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}
