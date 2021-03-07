/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2013-2019 CERN
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

#include <bitmaps.h>
#include <filehistory.h>
#include <kiface_i.h>
#include <menus_helpers.h>
#include <pgm_base.h>
#include <tool/action_menu.h>
#include <tool/tool_manager.h>
#include <widgets/wx_menubar.h>

#include "pl_editor_frame.h"
#include "pl_editor_id.h"
#include "tools/pl_actions.h"
#include "tools/pl_selection_tool.h"


void PL_EDITOR_FRAME::ReCreateMenuBar()
{
    PL_SELECTION_TOOL* selTool = m_toolManager->GetTool<PL_SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    WX_MENUBAR* menuBar    = new WX_MENUBAR();

    static ACTION_MENU* openRecentMenu;  // Open Recent submenu, static to remember this menu
    FILE_HISTORY&       recentFiles = GetFileHistory();

    // Create the menu if it does not exist. Adding a file to/from the history
    // will automatically refresh the menu.
    if( !openRecentMenu )
    {
        openRecentMenu = new ACTION_MENU( false );
        openRecentMenu->SetTool( selTool );
        openRecentMenu->SetTitle( _( "Open Recent" ) );
        openRecentMenu->SetIcon( recent_xpm );

        recentFiles.UseMenu( openRecentMenu );
        recentFiles.AddFilesToMenu();
    }

    //-- File menu -------------------------------------------------------
    //
    ACTION_MENU* fileMenu = new ACTION_MENU( false, selTool );

    fileMenu->Add( ACTIONS::doNew );
    fileMenu->Add( ACTIONS::open );

    wxMenuItem* item = fileMenu->Add( openRecentMenu );

    // Add the file menu condition here since it needs the item ID for the submenu
    ACTION_CONDITIONS cond;
    cond.Enable( FILE_HISTORY::FileHistoryNotEmpty( recentFiles ) );
    RegisterUIUpdateHandler( item->GetId(), cond );

    fileMenu->AppendSeparator();
    fileMenu->Add( ACTIONS::save );
    fileMenu->Add( ACTIONS::saveAs );

    fileMenu->AppendSeparator();
    fileMenu->Add( ACTIONS::print );

    fileMenu->AppendSeparator();
    fileMenu->AddQuitOrClose( &Kiface(), _( "Drawing Sheet Editor" ) );

    //-- Edit menu -------------------------------------------------------
    //
    ACTION_MENU* editMenu = new ACTION_MENU( false, selTool );

    editMenu->Add( ACTIONS::undo );
    editMenu->Add( ACTIONS::redo );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::cut );
    editMenu->Add( ACTIONS::copy );
    editMenu->Add( ACTIONS::paste );
    editMenu->Add( ACTIONS::doDelete );

    //-- View menu -------------------------------------------------------
    //
    ACTION_MENU* viewMenu = new ACTION_MENU( false, selTool );

    viewMenu->Add( ACTIONS::zoomInCenter );
    viewMenu->Add( ACTIONS::zoomOutCenter );
    viewMenu->Add( ACTIONS::zoomFitScreen );
    viewMenu->Add( ACTIONS::zoomTool );
    viewMenu->Add( ACTIONS::zoomRedraw );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::toggleGrid,          ACTION_MENU::CHECK );

    // Units submenu
    ACTION_MENU* unitsSubMenu = new ACTION_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( unit_mm_xpm );
    unitsSubMenu->Add( ACTIONS::inchesUnits,      ACTION_MENU::CHECK );
    unitsSubMenu->Add( ACTIONS::milsUnits,        ACTION_MENU::CHECK );
    unitsSubMenu->Add( ACTIONS::millimetersUnits, ACTION_MENU::CHECK );
    viewMenu->Add( unitsSubMenu );

    viewMenu->Add( ACTIONS::toggleCursorStyle,   ACTION_MENU::CHECK );

    viewMenu->AppendSeparator();
    viewMenu->Add( PL_ACTIONS::previewSettings );

#ifdef __APPLE__
    // Add a separator only on macOS because the OS adds menu items to the view menu after ours
    viewMenu->AppendSeparator();
#endif

    //-- Place menu -------------------------------------------------------
    //
    ACTION_MENU* placeMenu = new ACTION_MENU( false, selTool );

    placeMenu->Add( PL_ACTIONS::drawLine );
    placeMenu->Add( PL_ACTIONS::drawRectangle );
    placeMenu->Add( PL_ACTIONS::placeText );
    placeMenu->Add( PL_ACTIONS::placeImage );

    placeMenu->AppendSeparator();
    placeMenu->Add( PL_ACTIONS::appendImportedWorksheet );

    //-- Inspector menu -------------------------------------------------------
    //
    ACTION_MENU* inspectorMenu = new ACTION_MENU( false, selTool );
    inspectorMenu->Add( PL_ACTIONS::showInspector );

    //-- Preferences menu --------------------------------------------------
    //
    ACTION_MENU* preferencesMenu = new ACTION_MENU( false, selTool );

    preferencesMenu->Add( _( "Preferences..." ) + "\tCtrl+,",
                          _( "Show preferences for all open tools" ),
                          wxID_PREFERENCES,
                          preference_xpm );

    // Language submenu
    AddMenuLanguageList( preferencesMenu, selTool );

    //-- Menubar -----------------------------------------------------------
    //
    menuBar->Append( fileMenu,        _( "&File" ) );
    menuBar->Append( editMenu,        _( "&Edit" ) );
    menuBar->Append( viewMenu,        _( "&View" ) );
    menuBar->Append( placeMenu,       _( "&Place" ) );
    menuBar->Append( inspectorMenu,   _( "&Inspect" ) );
    menuBar->Append( preferencesMenu, _( "P&references" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}
