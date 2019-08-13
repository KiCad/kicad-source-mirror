/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <menus_helpers.h>
#include <tool/actions.h>
#include <tool/common_control.h>
#include <tool/conditional_menu.h>
#include <tool/tool_manager.h>

#include <cvpcb_mainframe.h>
#include <tools/cvpcb_actions.h>


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

    fileMenu->AddItem( CVPCB_ACTIONS::saveAssociations, SELECTION_CONDITIONS::ShowAlways );
    fileMenu->AddSeparator();
    fileMenu->AddClose( _( "Assign Footprints" ) );

    fileMenu->Resolve();

    //-- Preferences menu -----------------------------------------------
    //
    CONDITIONAL_MENU* editMenu = new CONDITIONAL_MENU( false, tool );

    auto enableUndoCondition = [ this ] ( const SELECTION& sel )
    {
        return m_undoList.size() > 0;
    };
    auto enableRedoCondition = [ this ] ( const SELECTION& sel )
    {
        return m_redoList.size() > 0;
    };

    editMenu->AddItem( ACTIONS::undo,  enableUndoCondition );
    editMenu->AddItem( ACTIONS::redo,  enableRedoCondition );
    editMenu->AddSeparator();
    editMenu->AddItem( ACTIONS::cut,   SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( ACTIONS::copy,  SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( ACTIONS::paste, SELECTION_CONDITIONS::ShowAlways );

    editMenu->Resolve();

    //-- Preferences menu -----------------------------------------------
    //
    CONDITIONAL_MENU* prefsMenu = new CONDITIONAL_MENU( false, tool );

    prefsMenu->AddItem( ACTIONS::configurePaths,         SELECTION_CONDITIONS::ShowAlways );
    prefsMenu->AddItem( ACTIONS::showFootprintLibTable,  SELECTION_CONDITIONS::ShowAlways );
    prefsMenu->AddItem( wxID_PREFERENCES,
                        _( "Preferences...\tCTRL+," ),
                        _( "Show preferences for all open tools" ),
                        preference_xpm,                  SELECTION_CONDITIONS::ShowAlways );
    prefsMenu->AddSeparator();
    prefsMenu->AddItem( CVPCB_ACTIONS::showEquFileTable, SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddSeparator();
    AddMenuLanguageList( prefsMenu, tool );

    prefsMenu->Resolve();

    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( prefsMenu, _( "&Preferences" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}
