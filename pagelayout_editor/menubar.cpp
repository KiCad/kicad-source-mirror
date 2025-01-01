/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <file_history.h>
#include <kiface_base.h>
#include <tool/action_manager.h>
#include <tool/action_menu.h>
#include <tool/tool_manager.h>
#include <widgets/wx_menubar.h>

#include "pl_editor_frame.h"
#include "pl_editor_id.h"
#include "tools/pl_actions.h"
#include "tools/pl_selection_tool.h"


void PL_EDITOR_FRAME::doReCreateMenuBar()
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
        openRecentMenu = new ACTION_MENU( false, selTool );
        openRecentMenu->SetIcon( BITMAPS::recent );

        recentFiles.UseMenu( openRecentMenu );
        recentFiles.AddFilesToMenu();
    }

    // Ensure the title is up to date after changing language
    openRecentMenu->SetTitle( _( "Open Recent" ) );
    recentFiles.UpdateClearText( openRecentMenu, _( "Clear Recent Files" ) );

    //-- File menu -------------------------------------------------------
    //
    ACTION_MENU* fileMenu = new ACTION_MENU( false, selTool );

    fileMenu->Add( ACTIONS::doNew );
    fileMenu->Add( ACTIONS::open );

    wxMenuItem* item = fileMenu->Add( openRecentMenu->Clone() );

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
    fileMenu->AddClose( _( "Drawing Sheet Editor" ) );
    fileMenu->AddQuit( _( "Drawing Sheet Editor" ) );

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
    placeMenu->Add( PL_ACTIONS::appendImportedDrawingSheet );

    //-- Inspector menu -------------------------------------------------------
    //
    ACTION_MENU* inspectorMenu = new ACTION_MENU( false, selTool );
    inspectorMenu->Add( PL_ACTIONS::showInspector );

    //-- Preferences menu --------------------------------------------------
    //
    ACTION_MENU* preferencesMenu = new ACTION_MENU( false, selTool );

    preferencesMenu->Add( ACTIONS::openPreferences );

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
