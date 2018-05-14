/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2018 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2013 CERN
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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
 * @file pagelayout_editor/menubar.cpp
 * @brief (Re)Create the main menubar for Pl_Editor
 */


#include <kiface_i.h>
#include <menus_helpers.h>
#include <pgm_base.h>

#include "hotkeys.h"
#include "pl_editor_frame.h"
#include "pl_editor_id.h"


void PL_EDITOR_FRAME::ReCreateMenuBar()
{
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    wxMenuBar*  menuBar = new wxMenuBar();

    wxString msg;
    static wxMenu* openRecentMenu;  // Open Recent submenu,
                                    // static to remember this menu

    // Before deleting, remove the menus managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentMenu )
        Kiface().GetFileHistory().RemoveMenu( openRecentMenu );

    // Recreate all menus:

    // File Menu:
    wxMenu* fileMenu = new wxMenu;

    msg = AddHotkeyName( _( "&New" ), PlEditorHokeysDescr, HK_NEW );
    AddMenuItem( fileMenu, wxID_NEW, msg,
                 _( "Create new page layout design" ),
                 KiBitmap( new_page_layout_xpm ) );

    msg = AddHotkeyName( _( "&Open..." ), PlEditorHokeysDescr, HK_OPEN );
    AddMenuItem( fileMenu, wxID_OPEN, msg,
                 _( "Open an existing page layout design file" ),
                 KiBitmap(  open_page_layout_xpm ) );

    openRecentMenu = new wxMenu();
    Kiface().GetFileHistory().UseMenu( openRecentMenu );
    Kiface().GetFileHistory().AddFilesToMenu();
    AddMenuItem( fileMenu, openRecentMenu, wxID_ANY, _( "Open &Recent" ),
                 _( "Open recent page layout design file" ),
                 KiBitmap(  recent_xpm ) );

    fileMenu->AppendSeparator();

    msg = AddHotkeyName( _( "&Save" ), PlEditorHokeysDescr, HK_SAVE );
    AddMenuItem( fileMenu, wxID_SAVE, msg,
                 _( "Save current page layout design file" ),
                 KiBitmap( save_xpm ) );

    msg = AddHotkeyName( _( "Save &As..." ), PlEditorHokeysDescr, HK_SAVEAS );
    AddMenuItem( fileMenu, wxID_SAVEAS, msg,
                 _( "Save current page layout design file with a different name" ),
                 KiBitmap( save_as_xpm ) );

    fileMenu->AppendSeparator();

    msg = AddHotkeyName( _( "&Print..." ), PlEditorHokeysDescr, HK_PRINT );
    AddMenuItem( fileMenu, wxID_PRINT, msg, KiBitmap( print_button_xpm ) );

    AddMenuItem( fileMenu, wxID_PREVIEW, _( "Print Pre&view..." ), KiBitmap( print_button_xpm ) );

    fileMenu->AppendSeparator();

    AddMenuItem( fileMenu, wxID_EXIT, _( "&Close" ),
                 _( "Close Page Layout Editor" ),
                 KiBitmap( exit_xpm ) );


    // Edit Menu:
    wxMenu* editMenu = new wxMenu;

    msg = AddHotkeyName( _( "Undo" ), PlEditorHokeysDescr, HK_UNDO );
    AddMenuItem( editMenu, wxID_UNDO, msg, wxEmptyString, KiBitmap( undo_xpm ) );

    msg = AddHotkeyName( _( "Redo" ), PlEditorHokeysDescr, HK_REDO );
    AddMenuItem( editMenu, wxID_REDO, msg, wxEmptyString, KiBitmap( redo_xpm ) );

    editMenu->AppendSeparator();

    AddMenuItem( editMenu, wxID_CUT, _( "Delete" ), wxEmptyString, KiBitmap( delete_xpm ) );


    // View Menu:
    wxMenu* viewMenu = new wxMenu;

    msg = AddHotkeyName( _( "Zoom In" ), PlEditorHokeysDescr, HK_ZOOM_IN, IS_ACCELERATOR );
    AddMenuItem( viewMenu, ID_ZOOM_IN, msg, wxEmptyString, KiBitmap( zoom_in_xpm ) );

    msg = AddHotkeyName( _( "Zoom Out" ), PlEditorHokeysDescr, HK_ZOOM_OUT, IS_ACCELERATOR );
    AddMenuItem( viewMenu, ID_ZOOM_OUT, msg, wxEmptyString, KiBitmap( zoom_out_xpm ) );

    msg = AddHotkeyName( _( "Zoom to Fit" ), PlEditorHokeysDescr, HK_ZOOM_AUTO );
    AddMenuItem( viewMenu, ID_ZOOM_PAGE, msg, wxEmptyString, KiBitmap( zoom_fit_in_page_xpm ) );

    msg = AddHotkeyName( _( "Zoom to Selection" ), PlEditorHokeysDescr, HK_ZOOM_SELECTION );
    AddMenuItem( viewMenu, ID_MENU_ZOOM_SELECTION, msg, wxEmptyString, KiBitmap( zoom_area_xpm ) );

    viewMenu->AppendSeparator();

    AddMenuItem( viewMenu, ID_MENU_SWITCH_BGCOLOR,
                 GetDrawBgColor() == WHITE ?  _( "&Background Black" ) : _( "&Background White" ),
                 wxEmptyString, KiBitmap( palette_xpm ) );

    AddMenuItem( viewMenu, ID_MENU_GRID_ONOFF,
                 IsGridVisible() ? _( "Hide &Grid" ) :  _( "Show &Grid" ),
                 wxEmptyString, KiBitmap( grid_xpm ) );

    viewMenu->AppendSeparator();

    msg = AddHotkeyName( _( "Redraw View" ), PlEditorHokeysDescr, HK_ZOOM_REDRAW );
    AddMenuItem( viewMenu, ID_ZOOM_REDRAW, msg, wxEmptyString, KiBitmap( zoom_redraw_xpm ) );


    // Place Menu:
    wxMenu* placeMenu = new wxMenu;

    AddMenuItem( placeMenu, ID_POPUP_ITEM_ADD_LINE, _( "&Line..." ),
                 wxEmptyString, KiBitmap( add_dashed_line_xpm ) );

    AddMenuItem( placeMenu, ID_POPUP_ITEM_ADD_RECT, _( "&Rectangle..." ),
                 wxEmptyString, KiBitmap( add_rectangle_xpm ) );

    AddMenuItem( placeMenu, ID_POPUP_ITEM_ADD_TEXT, _( "&Text..." ),
                 wxEmptyString, KiBitmap( text_xpm ) );

    AddMenuItem( placeMenu, ID_POPUP_ITEM_ADD_BITMAP, _( "&Bitmap..." ),
                 wxEmptyString, KiBitmap( image_xpm ) );

    placeMenu->AppendSeparator();

    AddMenuItem( placeMenu, ID_APPEND_DESCR_FILE, _( "&Append Existing Page Layout Design File..." ),
                 _( "Append an existing page layout design file to current file" ),
                 KiBitmap( pagelayout_load_xpm ) );


    // Menu for preferences
    wxMenu* preferencesMenu = new wxMenu;

    AddMenuItem( preferencesMenu, wxID_PREFERENCES,  _( "&Preferences..." ),
                 _( "Show preferences for all open tools" ),
                 KiBitmap( preference_xpm ) );

    // Language submenu
    Pgm().AddMenuLanguageList( preferencesMenu );

    // Menu Help
    wxMenu* helpMenu = new wxMenu;

    // Contents
    AddMenuItem( helpMenu, wxID_HELP, _( "Page Layout Editor &Manual" ),
                 _( "Open the Page Layout Editor Manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu, wxID_INDEX, _( "&Getting Started in KiCad" ),
                 _( "Open \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    msg = AddHotkeyName( _( "&List Hotkeys" ), PlEditorHokeysDescr, HK_HELP );
    AddMenuItem( helpMenu, ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST, msg,
                 _( "Displays the current hotkeys list and corresponding commands" ),
                 KiBitmap( hotkeys_xpm ) );

    helpMenu->AppendSeparator();
    AddMenuItem( helpMenu, ID_HELP_GET_INVOLVED, _( "Get &Involved" ),
                 _( "Contribute to KiCad (opens a web browser)" ),
                 KiBitmap( info_xpm ) );

    // Separator
    helpMenu->AppendSeparator();

    // About Kicad
    AddMenuItem( helpMenu, wxID_ABOUT, _( "&About KiCad" ), wxEmptyString, KiBitmap( about_xpm ) );

    // Append menus to the menubar
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( preferencesMenu, _( "P&references" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}
