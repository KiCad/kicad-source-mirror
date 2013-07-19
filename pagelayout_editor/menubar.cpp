/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pl_editor/menubar.cpp
 * @brief (Re)Create the main menubar for Pl_Editor
 */
#include <fctsys.h>

#include <appl_wxstruct.h>

#include <pl_editor_frame.h>
#include <pl_editor_id.h>
#include <hotkeys.h>
#include <menus_helpers.h>


void PL_EDITOR_FRAME::ReCreateMenuBar( void )
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
                 wxEmptyString, KiBitmap( pagelayout_new_xpm ) );
    AddMenuItem( fileMenu, wxID_OPEN,
                 _( "Load &Page Layout File" ),
                 wxEmptyString, KiBitmap(  pagelayout_load_xpm ) );
    AddMenuItem( fileMenu, ID_LOAD_DEFAULT_PAGE_LAYOUT,
                 _( "Load &Default Page Layout" ),
                 wxEmptyString, KiBitmap(  pagelayout_load_default_xpm ) );

    // Recent gerber files
    static wxMenu* openRecentMenu;

    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentMenu )
        wxGetApp().GetFileHistory().RemoveMenu( openRecentMenu );

    openRecentMenu = new wxMenu();
    wxGetApp().GetFileHistory().UseMenu( openRecentMenu );
    wxGetApp().GetFileHistory().AddFilesToMenu();
    AddMenuItem( fileMenu, openRecentMenu,
                 wxID_ANY, _( "Open &Recent Page Layout File" ),
                 wxEmptyString, KiBitmap(  pagelayout_recent_xpm ) );

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
                 _( "E&xit" ),
                 _( "Quit Pl_Editor" ),
                 KiBitmap( exit_xpm ) );

    // Menu for configuration and preferences
    wxMenu* configMenu = new wxMenu;

    // Options (Preferences on WXMAC)
#ifdef __WXMAC__
    configMenu->Append(wxID_PREFERENCES);
#else
    AddMenuItem( configMenu,
                 wxID_PREFERENCES,
                 _( "&Options" ),
                 _( "Set options to draw items" ),
                 KiBitmap( preference_xpm ) );
#endif // __WXMAC__


    // Language submenu
    wxGetApp().AddMenuLanguageList( configMenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( configMenu );

    // Menu miscellaneous
    wxMenu* miscellaneousMenu = new wxMenu;

    // Separator
    miscellaneousMenu->AppendSeparator();

    // Text editor
    AddMenuItem( miscellaneousMenu,
                 ID_MENU_PL_EDITOR_SELECT_PREFERED_EDITOR,
                 _( "&Text Editor" ),
                 _( "Select your preferred text editor" ),
                 KiBitmap( editor_xpm ) );

    // Menu Help
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contents
    AddMenuItem( helpMenu,
                 wxID_HELP,
                 _( "&Contents" ),
                 _( "Open the GerbView handbook" ),
                 KiBitmap( help_xpm ) );

    // About GerbView
    AddMenuItem( helpMenu,
                 wxID_ABOUT,
                 _( "&About Pl_Editor" ),
                 _( "About page layout description editor" ),
                 KiBitmap( online_help_xpm ) );

    // Append menus to the menubar
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( configMenu, _( "&Preferences" ) );
    menuBar->Append( miscellaneousMenu, _( "&Miscellaneous" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}
