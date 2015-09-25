/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2013 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file gerbview/menubar.cpp
 * @brief (Re)Create the main menubar for GerbView
 */
#include <fctsys.h>

#include <pgm_base.h>
#include <kiface_i.h>
#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerbview_id.h>
#include <hotkeys.h>
#include <menus_helpers.h>


void GERBVIEW_FRAME::ReCreateMenuBar()
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
    AddMenuItem( fileMenu,
                 wxID_FILE,
                 _( "Load &Gerber File" ),
                 _( "Load a new Gerber file on the current layer. Previous data will be deleted" ),
                 KiBitmap( gerber_file_xpm ) );

    // Excellon
    AddMenuItem( fileMenu,
                 ID_GERBVIEW_LOAD_DRILL_FILE,
                 _( "Load &EXCELLON Drill File" ),
                 _( "Load excellon drill file" ),
                 KiBitmap( gerbview_drill_file_xpm ) );

    // Recent gerber files
    static wxMenu* openRecentGbrMenu;

    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentGbrMenu )
        Kiface().GetFileHistory().RemoveMenu( openRecentGbrMenu );

    openRecentGbrMenu = new wxMenu();

    Kiface().GetFileHistory().UseMenu( openRecentGbrMenu );
    Kiface().GetFileHistory().AddFilesToMenu();

    AddMenuItem( fileMenu, openRecentGbrMenu,
                 wxID_ANY,
                 _( "Open &Recent Gerber File" ),
                 _( "Open a recent opened Gerber file" ),
                 KiBitmap( gerber_recent_files_xpm ) );

    // Recent drill files
    static wxMenu* openRecentDrlMenu;

    if( openRecentDrlMenu )
        m_drillFileHistory.RemoveMenu( openRecentDrlMenu );

    openRecentDrlMenu = new wxMenu();
    m_drillFileHistory.UseMenu( openRecentDrlMenu );
    m_drillFileHistory.AddFilesToMenu( );
    AddMenuItem( fileMenu, openRecentDrlMenu,
                 wxID_ANY,
                 _( "Open Recent Dri&ll File" ),
                 _( "Open a recent opened drill file" ),
                 KiBitmap( gerbview_open_recent_drill_files_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Clear all
    AddMenuItem( fileMenu,
                 ID_GERBVIEW_ERASE_ALL,
                 _( "Clear &All" ),
                 _( "Clear all layers. All data will be deleted" ),
                 KiBitmap( gerbview_clear_layers_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Export to Pcbnew
    AddMenuItem( fileMenu,
                 ID_GERBVIEW_EXPORT_TO_PCBNEW,
                 _( "E&xport to Pcbnew" ),
                 _( "Export data in Pcbnew format" ),
                 KiBitmap( export_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Print
    AddMenuItem( fileMenu,
                 wxID_PRINT,
                 _( "&Print" ),
                 _( "Print gerber" ),
                 KiBitmap( print_button_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Exit
    AddMenuItem( fileMenu,
                 wxID_EXIT,
                 _( "&Close" ),
                 _( "Close GerbView" ),
                 KiBitmap( exit_xpm ) );

    // Menu for configuration and preferences
    wxMenu* configMenu = new wxMenu;

    // Hide layer manager
    AddMenuItem( configMenu,
                 ID_MENU_GERBVIEW_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
                 _( "Hide &Layers Manager" ),
                 m_show_layer_manager_tools ?
                           _( "Hide &Layers Manager" ) : _("Show &Layers Manager" ),
                 KiBitmap( layers_manager_xpm ) );

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
    Pgm().AddMenuLanguageList( configMenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( configMenu );

    // Menu miscellaneous
    wxMenu* miscellaneousMenu = new wxMenu;

    // List dcodes
    AddMenuItem( miscellaneousMenu,
                 ID_GERBVIEW_SHOW_LIST_DCODES,
                 _( "&List DCodes" ),
                 _( "List and edit D-codes" ),
                 KiBitmap( show_dcodenumber_xpm ) );

    // Show source
    AddMenuItem( miscellaneousMenu,
                 ID_GERBVIEW_SHOW_SOURCE,
                 _( "&Show Source" ),
                 _( "Show source file for the current layer" ),
                 KiBitmap( tools_xpm ) );

    // Separator
    miscellaneousMenu->AppendSeparator();

    // Clear layer
    AddMenuItem( miscellaneousMenu,
                 ID_GERBVIEW_GLOBAL_DELETE,
                 _( "&Clear Layer" ),
                 _( "Clear current layer" ),
                 KiBitmap( general_deletions_xpm ) );

    // Separator
    miscellaneousMenu->AppendSeparator();

    // Text editor
    AddMenuItem( miscellaneousMenu,
                 ID_MENU_GERBVIEW_SELECT_PREFERED_EDITOR,
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
                 _( "Gerbview &Manual" ),
                 _( "Open the GerbView Manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu,
                 wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    // About Kicad
    AddMenuItem( helpMenu,
                 wxID_ABOUT,
                 _( "&About Kicad" ),
                 _( "About Kicad" ),
                 KiBitmap( info_xpm ) );

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
