/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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

    // Menu File:
    wxMenu* fileMenu = new wxMenu;

    // Load
    AddMenuItem( fileMenu, wxID_NEW,
                 _( "&New Page Layout Design" ),
                 wxEmptyString, KiBitmap( new_generic_xpm ) );
    AddMenuItem( fileMenu, wxID_OPEN,
                 _( "Load Page Layout &File" ),
                 wxEmptyString, KiBitmap(  pagelayout_load_xpm ) );
    AddMenuItem( fileMenu, ID_LOAD_DEFAULT_PAGE_LAYOUT,
                 _( "Load &Default Page Layout" ),
                 wxEmptyString, KiBitmap(  pagelayout_load_xpm ) );

    // Recent gerber files
    static wxMenu* openRecentMenu;

    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentMenu )
        Kiface().GetFileHistory().RemoveMenu( openRecentMenu );

    openRecentMenu = new wxMenu();

    Kiface().GetFileHistory().UseMenu( openRecentMenu );
    Kiface().GetFileHistory().AddFilesToMenu();

    AddMenuItem( fileMenu, openRecentMenu,
                 wxID_ANY, _( "Open &Recent Page Layout File" ),
                 wxEmptyString, KiBitmap(  recent_xpm ) );

     fileMenu->AppendSeparator();
    // Save current sheet
    AddMenuItem( fileMenu, wxID_SAVE,
                 _( "&Save Page Layout Design" ),
                 wxEmptyString, KiBitmap( save_xpm ) );

    // Save current sheet as
    AddMenuItem( fileMenu, wxID_SAVEAS,
                 _( "Save Page Layout Design &As" ),
                 wxEmptyString, KiBitmap( save_as_xpm ) );

    // Print
    fileMenu->AppendSeparator();
    AddMenuItem( fileMenu, wxID_PRINT, _( "&Print" ),
                 wxEmptyString, KiBitmap( print_button_xpm ) );
    AddMenuItem( fileMenu, wxID_PREVIEW, _( "Print Pre&view" ),
                 wxEmptyString, KiBitmap( print_button_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Exit
    AddMenuItem( fileMenu, wxID_EXIT,
                 _( "&Close" ),
                 _( "&Close Page Layout Editor" ),
                 KiBitmap( exit_xpm ) );

    // Menu for preferences
    wxMenu* preferencesMenu = new wxMenu;

    AddMenuItem( preferencesMenu,
                 ID_MENU_SWITCH_BGCOLOR,
                 GetDrawBgColor() == WHITE ?
                 _( "&Background Black" ) : _( "&Background White" ),
                 wxEmptyString, KiBitmap( palette_xpm ) );

    AddMenuItem( preferencesMenu,
                 ID_MENU_GRID_ONOFF,
                 IsGridVisible() ? _( "Hide &Grid" ) :  _( "Show &Grid" ),
                 wxEmptyString, KiBitmap( grid_xpm ) );

    // Text editor selection
    AddMenuItem( preferencesMenu,
                 ID_MENU_PL_EDITOR_SELECT_PREFERED_EDITOR,
                 _( "&Text Editor" ),
                 _( "Select your preferred text editor" ),
                 KiBitmap( editor_xpm ) );

    // Language submenu
    Pgm().AddMenuLanguageList( preferencesMenu );

    // Icons options submenu
    AddMenuIconsOptions( preferencesMenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( preferencesMenu );

    // Menu Help
    wxMenu* helpMenu = new wxMenu;

    // Contents
    AddMenuItem( helpMenu,
                 wxID_HELP,
                 _( "Page Layout Editor &Manual" ),
                 _( "Open the Page Layout Editor Manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu,
                 wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    AddMenuItem( helpMenu, ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST,
                 _( "&List Hotkeys" ),
                 _( "Displays the current hotkeys list and corresponding commands" ),
                 KiBitmap( hotkeys_xpm ) );

    helpMenu->AppendSeparator();
    AddMenuItem( helpMenu, ID_HELP_GET_INVOLVED,
                 _( "Get &Involved" ),
                 _( "Contribute to KiCad (opens a web browser)" ),
                 KiBitmap( info_xpm ) );

    // Separator
    helpMenu->AppendSeparator();

    // About Kicad
    AddMenuItem( helpMenu,
                 wxID_ABOUT,
                 _( "&About Kicad" ),
                 _( "About KiCad" ),
                 KiBitmap( about_xpm ) );

    // Append menus to the menubar
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( preferencesMenu, _( "&Preferences" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}
