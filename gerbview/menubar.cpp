/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "gerbview_frame.h"

#include <bitmaps.h>
#include "gerbview_id.h"
#include <kiface_base.h>
#include <tool/action_manager.h>
#include <tool/action_menu.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tools/gerbview_actions.h>
#include <tools/gerbview_selection_tool.h>
#include <widgets/wx_menubar.h>


void GERBVIEW_FRAME::doReCreateMenuBar()
{
    GERBVIEW_SELECTION_TOOL* selTool = m_toolManager->GetTool<GERBVIEW_SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    WX_MENUBAR* menuBar    = new WX_MENUBAR();

    //-- File menu -------------------------------------------------------
    //
    ACTION_MENU*   fileMenu = new ACTION_MENU( false, selTool );
    static ACTION_MENU* openRecentGbrMenu;
    static ACTION_MENU* openRecentDrlMenu;
    static ACTION_MENU* openRecentJobMenu;
    static ACTION_MENU* openRecentZipMenu;

    FILE_HISTORY& recentGbrFiles = GetFileHistory();

#define FileHistoryCond( x ) ACTION_CONDITIONS().Enable( FILE_HISTORY::FileHistoryNotEmpty( x ) )


    // Create the gerber file menu if it does not exist. Adding a file to/from the history
    // will automatically refresh the menu.
    if( !openRecentGbrMenu )
    {
        openRecentGbrMenu = new ACTION_MENU( false, selTool );
        openRecentGbrMenu->SetIcon( BITMAPS::recent );

        recentGbrFiles.SetClearText( _( "Clear Recent Gerber Files" ) );
        recentGbrFiles.UseMenu( openRecentGbrMenu );
        recentGbrFiles.AddFilesToMenu();
    }

    // Ensure the title is up to date after changing language
    openRecentGbrMenu->SetTitle( _( "Open Recent Gerber File" ) );
    recentGbrFiles.UpdateClearText( openRecentGbrMenu, _( "Clear Recent Gerber Files" ) );

    fileMenu->Add( GERBVIEW_ACTIONS::openAutodetected );
    fileMenu->Add( GERBVIEW_ACTIONS::openGerber );
    wxMenuItem* gbrItem = fileMenu->Add( openRecentGbrMenu->Clone() );
    RegisterUIUpdateHandler( gbrItem->GetId(), FileHistoryCond( recentGbrFiles) );


    // Create the drill file menu if it does not exist. Adding a file to/from the history
    // will automatically refresh the menu.
    if( !openRecentDrlMenu )
    {
        openRecentDrlMenu = new ACTION_MENU( false, selTool );
        openRecentDrlMenu->SetTitle( _( "Open Recent Drill File" ) );
        openRecentDrlMenu->SetIcon( BITMAPS::recent );

        m_drillFileHistory.UseMenu( openRecentDrlMenu );
        m_drillFileHistory.SetClearText( _( "Clear Recent Drill Files" ) );
        m_drillFileHistory.AddFilesToMenu();
    }

    // Ensure the title is up to date after changing language
    openRecentGbrMenu->SetTitle( _( "Open Recent Gerber File" ) );
    m_drillFileHistory.UpdateClearText( openRecentDrlMenu, _( "Clear Recent Drill Files" ) );

    fileMenu->Add( GERBVIEW_ACTIONS::openDrillFile );
    wxMenuItem* drillItem = fileMenu->Add( openRecentDrlMenu->Clone() );
    RegisterUIUpdateHandler( drillItem->GetId(), FileHistoryCond( m_drillFileHistory ) );


    // Create the job file menu if it does not exist. Adding a file to/from the history
    // will automatically refresh the menu.
    if( !openRecentJobMenu )
    {
        openRecentJobMenu = new ACTION_MENU( false, selTool );
        openRecentJobMenu->SetIcon( BITMAPS::recent );

        m_jobFileHistory.SetClearText( _( "Clear Recent Job Files" ) );
        m_jobFileHistory.UseMenu( openRecentJobMenu );
        m_jobFileHistory.AddFilesToMenu();
    }

    // Ensure the title is up to date after changing language
    openRecentJobMenu->SetTitle( _( "Open Recent Job File" ) );
    m_jobFileHistory.UpdateClearText( openRecentJobMenu, _( "Clear Recent Job Files" ) );

    fileMenu->Add( GERBVIEW_ACTIONS::openJobFile );
    wxMenuItem* jobItem = fileMenu->Add( openRecentJobMenu->Clone() );
    RegisterUIUpdateHandler( jobItem->GetId(), FileHistoryCond( m_jobFileHistory ) );


    // Create the zip file menu if it does not exist. Adding a file to/from the history
    // will automatically refresh the menu.
    if( !openRecentZipMenu )
    {
        openRecentZipMenu = new ACTION_MENU( false, selTool );
        openRecentZipMenu->SetIcon( BITMAPS::recent );

        m_zipFileHistory.UseMenu( openRecentZipMenu );
        m_zipFileHistory.SetClearText( _( "Clear Recent Zip Files" ) );
        m_zipFileHistory.AddFilesToMenu();
    }

    // Ensure the title is up to date after changing language
    openRecentZipMenu->SetTitle( _( "Open Recent Zip File" ) );
    m_zipFileHistory.UpdateClearText( openRecentZipMenu, _( "Clear Recent Zip Files" ) );

    fileMenu->Add( GERBVIEW_ACTIONS::openZipFile );
    wxMenuItem* zipItem = fileMenu->Add( openRecentZipMenu->Clone() );
    RegisterUIUpdateHandler( zipItem->GetId(), FileHistoryCond( m_zipFileHistory ) );

#undef FileHistoryCond

    fileMenu->AppendSeparator();
    fileMenu->Add( GERBVIEW_ACTIONS::clearAllLayers );
    fileMenu->Add( GERBVIEW_ACTIONS::reloadAllLayers );

    fileMenu->AppendSeparator();
    fileMenu->Add( GERBVIEW_ACTIONS::exportToPcbnew );

    fileMenu->AppendSeparator();
    fileMenu->Add( ACTIONS::print );

    fileMenu->AppendSeparator();
    fileMenu->AddQuitOrClose( &Kiface(), _( "Gerber Viewer" ) );


    //-- View menu -------------------------------------------------------
    //
    ACTION_MENU* viewMenu = new ACTION_MENU( false, selTool );

    viewMenu->Add( ACTIONS::zoomInCenter );
    viewMenu->Add( ACTIONS::zoomOutCenter );
    viewMenu->Add( ACTIONS::zoomFitScreen );
    viewMenu->Add( ACTIONS::zoomTool );
    viewMenu->Add( ACTIONS::zoomRedraw );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::toggleGrid,                       ACTION_MENU::CHECK );
    viewMenu->Add( ACTIONS::togglePolarCoords,                ACTION_MENU::CHECK );

#ifdef __APPLE__
    // Add a separator only on macOS because the OS adds menu items to the view menu after ours
    viewMenu->AppendSeparator();
#endif

    // Units submenu
    ACTION_MENU* unitsSubMenu = new ACTION_MENU( false, selTool );

    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( BITMAPS::unit_mm );
    unitsSubMenu->Add( ACTIONS::inchesUnits,                  ACTION_MENU::CHECK );
    unitsSubMenu->Add( ACTIONS::milsUnits,                    ACTION_MENU::CHECK );
    unitsSubMenu->Add( ACTIONS::millimetersUnits,             ACTION_MENU::CHECK );

    viewMenu->Add( unitsSubMenu );

    viewMenu->AppendSeparator();
    viewMenu->Add( GERBVIEW_ACTIONS::flashedDisplayOutlines,  ACTION_MENU::CHECK );
    viewMenu->Add( GERBVIEW_ACTIONS::linesDisplayOutlines,    ACTION_MENU::CHECK );
    viewMenu->Add( GERBVIEW_ACTIONS::polygonsDisplayOutlines, ACTION_MENU::CHECK );
    viewMenu->Add( GERBVIEW_ACTIONS::dcodeDisplay,            ACTION_MENU::CHECK );
    viewMenu->Add( GERBVIEW_ACTIONS::negativeObjectDisplay,   ACTION_MENU::CHECK );
    viewMenu->Add( GERBVIEW_ACTIONS::toggleForceOpacityMode,  ACTION_MENU::CHECK );
    viewMenu->Add( GERBVIEW_ACTIONS::toggleXORMode,           ACTION_MENU::CHECK );
    viewMenu->Add( ACTIONS::highContrastMode,                 ACTION_MENU::CHECK );
    viewMenu->Add( GERBVIEW_ACTIONS::flipGerberView,          ACTION_MENU::CHECK );

    viewMenu->AppendSeparator();
    viewMenu->Add( GERBVIEW_ACTIONS::toggleLayerManager,      ACTION_MENU::CHECK );

    //-- Tools menu -------------------------------------------------------
    //
    ACTION_MENU* toolsMenu = new ACTION_MENU( false, selTool );

    toolsMenu->Add( GERBVIEW_ACTIONS::showDCodes );
    toolsMenu->Add( GERBVIEW_ACTIONS::showSource );

    toolsMenu->Add( ACTIONS::measureTool );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( GERBVIEW_ACTIONS::clearLayer );


    //-- Preferences menu -----------------------------------------------
    //
    ACTION_MENU* preferencesMenu = new ACTION_MENU( false, selTool );

    preferencesMenu->Add( ACTIONS::openPreferences );

    preferencesMenu->AppendSeparator();
    AddMenuLanguageList( preferencesMenu, selTool );


    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu,        _( "&File" ) );
    menuBar->Append( viewMenu,        _( "&View" ) );
    menuBar->Append( toolsMenu,       _( "&Tools" ) );
    menuBar->Append( preferencesMenu, _( "&Preferences" ) );
    AddStandardHelpMenu( menuBar );

    // Associate the menu bar with the frame, if no previous menubar
    SetMenuBar( menuBar );
    delete oldMenuBar;
}
