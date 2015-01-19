/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file cvpcb/menubar.cpp
 * @brief (Re)Create the menubar for CvPcb
 */
#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <confirm.h>
#include <gestfich.h>
#include <menus_helpers.h>

#include <cvpcb.h>
#include <cvpcb_mainframe.h>
#include <cvpcb_id.h>

#include <common_help_msg.h>


/**
 * @brief (Re)Create the menubar for the CvPcb mainframe
 */
void CVPCB_MAINFRAME::ReCreateMenuBar()
{
    // Create and try to get the current  menubar
    wxMenuItem* item;
    wxMenuBar*  menuBar = GetMenuBar();

    if( ! menuBar )     // Delete all menus
        menuBar = new wxMenuBar();

    // Delete all existing menus so they can be rebuilt.
    // This allows language changes of the menu text on the fly.
    menuBar->Freeze();

    while( menuBar->GetMenuCount() )
        delete menuBar->Remove( 0 );

    // Recreate all menus:

    // Menu File:
    wxMenu* filesMenu = new wxMenu;

    // Open files can be used only outside a project, because opening a netlist
    // which is not the project netlist is a non sense.
    if( Kiface().IsSingle() )
    {
        AddMenuItem( filesMenu, ID_LOAD_PROJECT,
                     _( "&Open Netlist" ), LOAD_FILE_HELP, KiBitmap( open_document_xpm ) );

        // Open Recent submenu
        static wxMenu* openRecentMenu;

        // Add this menu to list menu managed by m_fileHistory
        // (the file history will be updated when adding/removing files in history
        if( openRecentMenu )
            Kiface().GetFileHistory().RemoveMenu( openRecentMenu );

        openRecentMenu = new wxMenu();

        Kiface().GetFileHistory().UseMenu( openRecentMenu );
        Kiface().GetFileHistory().AddFilesToMenu();

        AddMenuItem( filesMenu, openRecentMenu, -1,
                     _( "Open &Recent" ),
                     _( "Open recent netlist" ),
                     KiBitmap( open_project_xpm ) );

        // Separator
        filesMenu->AppendSeparator();
    }

    // Save the .cmp file
    AddMenuItem( filesMenu, wxID_SAVE,
                 _( "&Save Cmp File\tCtrl+S" ), SAVE_HLP_MSG, KiBitmap( save_xpm ) );

    // Save as the .cmp file
    AddMenuItem( filesMenu, wxID_SAVEAS,
                 _( "Save Cmp File &As...\tCtrl+Shift+S" ), SAVE_AS_HLP_MSG, KiBitmap( save_xpm ) );

    // Separator
    filesMenu->AppendSeparator();

    // Quit
    AddMenuItem( filesMenu, wxID_EXIT,
                 _( "&Quit" ), _( "Quit CvPcb" ),
                 KiBitmap( exit_xpm ) );

    // Menu Preferences:
    wxMenu* preferencesMenu = new wxMenu;

    AddMenuItem( preferencesMenu, ID_CVPCB_LIB_TABLE_EDIT,
                 _( "Edit Li&brary Table" ), _( "Setup footprint libraries" ),
                 KiBitmap( library_table_xpm ) );

    AddMenuItem( preferencesMenu, ID_CVPCB_EQUFILES_LIST_EDIT,
                 _( "Edit &Equ Files List" ),
                 _( "Setup equ files list (.equ files)\n"
                    "They are files which give the footprint name from the component value"),
                 KiBitmap( library_table_xpm ) );

    // Language submenu
    Pgm().AddMenuLanguageList( preferencesMenu );

    // Keep open on save
    item = new wxMenuItem( preferencesMenu, ID_CVPCB_CONFIG_KEEP_OPEN_ON_SAVE,
                           _( "Keep Open On Save" ),
                           _( "Prevent CvPcb from exiting after saving netlist file" ),
                           wxITEM_CHECK );
    preferencesMenu->Append( item );
    SETBITMAPS( window_close_xpm );

    // Separator
    preferencesMenu->AppendSeparator();
    AddMenuItem( preferencesMenu, ID_SAVE_PROJECT,
                 _( "&Save Project File" ),
                 _( "Save changes to the project configuration file" ),
                 KiBitmap( save_setup_xpm ) );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    // Manual Contents
    AddMenuItem( helpMenu, wxID_HELP, _( "&CvPcb Manual" ),
                 _( "Open CvPcb manual" ),
                 KiBitmap( online_help_xpm ) );

    // About CvPcb
    AddMenuItem( helpMenu, wxID_ABOUT,
                 _( "&About CvPcb" ),
                 _( "About CvPcb footprint selector" ),
                 KiBitmap( info_xpm ) );

    // Create the menubar and append all submenus
    menuBar->Append( filesMenu, _( "&File" ) );
    menuBar->Append( preferencesMenu, _( "&Preferences" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}
