/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2019 KiCad Developers, see change_log.txt for contributors.
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

#include <pgm_base.h>
#include <bitmaps.h>
#include <tool/conditional_menu.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tool/common_control.h>
#include "cvpcb_id.h"
#include "cvpcb_mainframe.h"


void CVPCB_MAINFRAME::ReCreateMenuBar()
{
    COMMON_CONTROL* tool = m_toolManager->GetTool<COMMON_CONTROL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    wxMenuBar*  menuBar = new wxMenuBar();

    //-- File menu -----------------------------------------------------------
    //
    CONDITIONAL_MENU*   fileMenu = new CONDITIONAL_MENU( false, tool );

    fileMenu->AddItem( ID_SAVE_PROJECT, 
                       _( "&Save Schematic\tCtrl+S" ),
                       _( "Save footprint associations in schematic symbol footprint fields" ),
                       save_xpm,                        SELECTION_CONDITIONS::ShowAlways );
    
    fileMenu->Resolve();

    //-- Preferences menu -----------------------------------------------
    //
    CONDITIONAL_MENU* prefsMenu = new CONDITIONAL_MENU( false, tool );

    prefsMenu->AddItem( ACTIONS::configurePaths,        SELECTION_CONDITIONS::ShowAlways );
    prefsMenu->AddItem( ACTIONS::showFootprintLibTable, SELECTION_CONDITIONS::ShowAlways );
    prefsMenu->AddItem( wxID_PREFERENCES,
                        _( "Preferences...\tCTRL+," ),
                        _( "Show preferences for all open tools" ),
                        preference_xpm,                 SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddSeparator();
    prefsMenu->AddItem( ID_CVPCB_EQUFILES_LIST_EDIT, 
                        _( "Footprint &Association Files..." ),
                        _( "Configure footprint association file (.equ) list.  These files are "
                           "used to automatically assign footprint names from symbol values." ),
                        library_table_xpm,              SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddSeparator();
    Pgm().AddMenuLanguageList( prefsMenu );

    prefsMenu->Resolve();

    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( prefsMenu, _( "&Preferences" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}
