/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2016 KiCad Developers, see change_log.txt for contributors.
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

#include <common_help_msg.h>
#include <kiface_i.h>
#include <menus_helpers.h>
#include <pgm_base.h>

#include "cvpcb.h"
#include "cvpcb_id.h"
#include "cvpcb_mainframe.h"


/**
 * @brief (Re)Create the menubar for the CvPcb mainframe
 */
void CVPCB_MAINFRAME::ReCreateMenuBar()
{
    // Create the current menubar if it does not yet exist
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

    // Save the footprints back into eeschema
    AddMenuItem( filesMenu, wxID_SAVE,
                 _( "&Save Footprint Associations\tCtrl+S" ),
                 _( "Save footprint association in schematic component footprint fields" ),
                 KiBitmap( save_xpm ) );

    // Separator
    filesMenu->AppendSeparator();

    // Quit
    AddMenuItem( filesMenu, wxID_EXIT,
                 _( "&Close" ), _( "Close CvPcb" ),
                 KiBitmap( exit_xpm ) );

    // Preferences Menu :
    wxMenu* preferencesMenu = new wxMenu;

    AddMenuItem( preferencesMenu, ID_CVPCB_LIB_TABLE_EDIT,
                 _( "Footprint &Libraries" ), _( "Configure footprint libraries" ),
                 KiBitmap( library_table_xpm ) );

    // Path configuration edit dialog.
    AddMenuItem( preferencesMenu,
                 ID_PREFERENCES_CONFIGURE_PATHS,
                 _( "Configure &Paths" ),
                 _( "Edit path configuration environment variables" ),
                 KiBitmap( editor_xpm ) );

    preferencesMenu->AppendSeparator();
    AddMenuItem( preferencesMenu, ID_CVPCB_EQUFILES_LIST_EDIT,
                 _( "Footprint &Association Files" ),
                 _( "Configure footprint association file (.equ) list."
                    "These files are used to automatically assign "
                    "the footprint name (FPID) from the component value" ),
                 KiBitmap( library_table_xpm ) );
    preferencesMenu->AppendSeparator();

    // Language submenu
    Pgm().AddMenuLanguageList( preferencesMenu );

    // Keep open on save data
    preferencesMenu->AppendSeparator();
    AddMenuItem( preferencesMenu, ID_CVPCB_CONFIG_KEEP_OPEN_ON_SAVE,
                 _( "&Keep Open On Save" ),
                 _( "Prevent CvPcb from exiting after saving netlist file" ),
                 KiBitmap( exit_xpm ),
                 wxITEM_CHECK );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;

    // Manual Contents
    AddMenuItem( helpMenu, wxID_HELP, _( "CvPcb &Manual" ),
                 _( "Open CvPcb Manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu,
                 wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    // About CvPcb
    AddMenuItem( helpMenu, wxID_ABOUT,
                 _( "&About Kicad" ),
                 _( "About KiCad" ),
                 KiBitmap( about_xpm ) );

    // Create the menubar and append all submenus
    menuBar->Append( filesMenu, _( "&Save" ) );
    menuBar->Append( preferencesMenu, _( "&Preferences" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}
